// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.devtools_bridge;

import android.util.Log;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Helper class designated for automatic and manual testing. Creates a pair of ClientSession and
 * ServerSession on separate and threads binds them through and adapters that makes call the calls
 * on correct threads (no serialization needed for communication).
 */
public class LocalSessionBridge {
    private static final String TAG = "LocalSessionBridge";

    private volatile int mDelayMs = 0;

    private final SessionDependencyFactory mFactory = new SessionDependencyFactory();

    private final ThreadedExecutor mServerExecutor = new ThreadedExecutor();
    private final ThreadedExecutor mClientExecutor = new ThreadedExecutor();

    private final ServerSessionMock mServerSession;
    private final ClientSessionMock mClientSession;

    private boolean mStarted = false;

    private final CountDownLatch mNegotiated = new CountDownLatch(2);
    private final CountDownLatch mControlChannelOpened = new CountDownLatch(2);
    private final CountDownLatch mClientAutoClosed = new CountDownLatch(1);
    private final CountDownLatch mServerAutoClosed = new CountDownLatch(1);
    private final CountDownLatch mTunnelConfirmed = new CountDownLatch(1);

    private int mServerAutoCloseTimeoutMs = -1;
    private int mClientAutoCloseTimeoutMs = -1;

    public LocalSessionBridge(String serverSocketName, String clientSocketName) throws IOException {
        mServerSession = new ServerSessionMock(serverSocketName);
        mClientSession = new ClientSessionMock(mServerSession, clientSocketName);
    }

    public void setMessageDeliveryDelayMs(int value) {
        mDelayMs = value;
    }

    void setClientAutoCloseTimeoutMs(int value) {
        assert !isStarted();

        mClientAutoCloseTimeoutMs = value;
    }

    void setServerAutoCloseTimeoutMs(int value) {
        assert !isStarted();

        mServerAutoCloseTimeoutMs = value;
    }

    public void dispose() {
        if (isStarted()) stop();

        mServerExecutor.dispose();
        mClientExecutor.dispose();
        mFactory.dispose();
    }

    public void start() {
        start(new RTCConfiguration());
    }

    public void start(final RTCConfiguration config) {
        if (mServerAutoCloseTimeoutMs >= 0)
            mServerSession.setAutoCloseTimeoutMs(mServerAutoCloseTimeoutMs);
        if (mClientAutoCloseTimeoutMs >= 0)
            mClientSession.setAutoCloseTimeoutMs(mClientAutoCloseTimeoutMs);
        mClientExecutor.runSynchronously(new Runnable() {
            @Override
            public void run() {
                mClientSession.start(config);
            }
        });
        mStarted = true;
    }

    private boolean isStarted() {
        return mStarted;
    }

    public void stop() {
        mServerExecutor.runSynchronously(new Runnable() {
            @Override
            public void run() {
                mServerSession.dispose();
            }
        });
        mClientExecutor.runSynchronously(new Runnable() {
            @Override
            public void run() {
                mClientSession.dispose();
            }
        });
        mStarted = false;
    }

    public void awaitNegotiated() throws InterruptedException {
        mNegotiated.await();
    }

    public void awaitControlChannelOpened() throws InterruptedException {
        mControlChannelOpened.await();
    }

    public void awaitClientAutoClosed() throws InterruptedException {
        mClientAutoClosed.await();
    }

    public void awaitServerAutoClosed() throws InterruptedException {
        mServerAutoClosed.await();
    }

    private class ServerSessionMock extends ServerSession {
        public ServerSessionMock(String serverSocketName) {
            super(mFactory, mServerExecutor, serverSocketName);
        }

        public void setAutoCloseTimeoutMs(int value) {
            mAutoCloseTimeoutMs = value;
        }

        @Override
        protected void onSessionNegotiated() {
            Log.d(TAG, "Server negotiated");
            mNegotiated.countDown();
            super.onSessionNegotiated();
        }

        @Override
        protected void onControlChannelOpened() {
            Log.d(TAG, "Server's control channel opened");
            super.onControlChannelOpened();
            mControlChannelOpened.countDown();
        }

        @Override
        protected void onIceCandidate(String candidate) {
            Log.d(TAG, "Server's ICE candidate: " + candidate);
            super.onIceCandidate(candidate);
        }

        @Override
        protected void closeSelf() {
            Log.d(TAG, "Server autoclosed");
            super.closeSelf();
            mServerAutoClosed.countDown();
        }

        @Override
        protected SocketTunnelServer createSocketTunnelServer(String serverSocketName) {
            SocketTunnelServer tunnel = super.createSocketTunnelServer(serverSocketName);
            Log.d(TAG, "Server tunnel created on " + serverSocketName);
            return tunnel;
        }
    }

    private class ClientSessionMock extends ClientSession {
        public ClientSessionMock(ServerSession serverSession, String clientSocketName)
                throws IOException {
            super(mFactory,
                  mClientExecutor,
                  createServerSessionProxy(serverSession),
                  clientSocketName);
        }

        public void setAutoCloseTimeoutMs(int value) {
            mAutoCloseTimeoutMs = value;
        }

        @Override
        protected void onSessionNegotiated() {
            Log.d(TAG, "Client negotiated");
            mNegotiated.countDown();
            super.onSessionNegotiated();
        }

        @Override
        protected void onControlChannelOpened() {
            Log.d(TAG, "Client's control channel opened");
            super.onControlChannelOpened();
            mControlChannelOpened.countDown();
        }

        @Override
        protected void onIceCandidate(String candidate) {
            Log.d(TAG, "Client's ICE candidate: " + candidate);
            super.onIceCandidate(candidate);
        }

        @Override
        protected void closeSelf() {
            Log.d(TAG, "Client autoclosed");
            super.closeSelf();
            mClientAutoClosed.countDown();
        }
    }

    /**
     * Implementation of SessionBase.Executor on top of ScheduledExecutorService.
     */
    public static final class ThreadedExecutor implements SessionBase.Executor {
        private final ScheduledExecutorService mExecutor =
                Executors.newSingleThreadScheduledExecutor();
        private final AtomicReference<Thread> mSessionThread = new AtomicReference<Thread>();

        @Override
        public SessionBase.Cancellable postOnSessionThread(int delayMs, Runnable runnable) {
            return new CancellableFuture(mExecutor.schedule(
                    new SessionThreadRunner(runnable), delayMs, TimeUnit.MILLISECONDS));
        }

        @Override
        public boolean isCalledOnSessionThread() {
            return Thread.currentThread() == mSessionThread.get();
        }

        public void runSynchronously(Runnable runnable) {
            try {
                mExecutor.submit(new SessionThreadRunner(runnable)).get();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            } catch (ExecutionException e) {
                throw new RuntimeException(e);
            }
        }

        public void dispose() {
            mExecutor.shutdownNow();
        }

        private class SessionThreadRunner implements Runnable {
            private final Runnable mRunnable;

            public SessionThreadRunner(Runnable runnable) {
                mRunnable = runnable;
            }

            @Override
            public void run() {
                Thread thread = mSessionThread.getAndSet(Thread.currentThread());
                assert thread == null;
                mRunnable.run();
                thread = mSessionThread.getAndSet(null);
                assert thread == Thread.currentThread();
            }
        }
    }

    private static final class CancellableFuture implements SessionBase.Cancellable {
        private final ScheduledFuture<?> mFuture;

        public CancellableFuture(ScheduledFuture<?> future) {
            mFuture = future;
        }

        @Override
        public void cancel() {
            mFuture.cancel(false);
        }
    }

    private ServerSessionProxy createServerSessionProxy(SessionBase.ServerSessionInterface proxee) {
        return new ServerSessionProxy(mServerExecutor, mClientExecutor, proxee, mDelayMs);
    }

    /**
     * Helper proxy that binds client and server sessions living on different executors.
     * Exchange java objects instead of serialized messages.
     */
    public static final class ServerSessionProxy implements SessionBase.ServerSessionInterface {
        private static final String SESSION_ID = "";
        private final SignalingReceiverProxy mProxy;

        public ServerSessionProxy(
                SessionBase.Executor serverExecutor, SessionBase.Executor clientExecutor,
                SessionBase.ServerSessionInterface proxee, int delayMs) {
            mProxy = new SignalingReceiverProxy(
                    serverExecutor, clientExecutor, new ServerSessionAdaptor(proxee), delayMs);
        }

        public SessionBase.Executor serverExecutor() {
            return mProxy.serverExecutor();
        }

        public SessionBase.Executor clientExecutor() {
            return mProxy.clientExecutor();
        }

        @Override
        public void startSession(
                RTCConfiguration config, String offer, SessionBase.NegotiationCallback callback) {
            mProxy.startSession(SESSION_ID, config, offer, callback);
        }

        @Override
        public void renegotiate(String offer, SessionBase.NegotiationCallback callback) {
            mProxy.renegotiate(SESSION_ID, offer, callback);
        }

        @Override
        public void iceExchange(
                List<String> clientCandidates, SessionBase.IceExchangeCallback callback) {
            mProxy.iceExchange(SESSION_ID, clientCandidates, callback);
        }
    }

    private static final class ServerSessionAdaptor implements SignalingReceiver {
        private final SessionBase.ServerSessionInterface mAdaptee;

        public ServerSessionAdaptor(SessionBase.ServerSessionInterface adaptee) {
            mAdaptee = adaptee;
        }

        @Override
        public void startSession(
                String sessionId, RTCConfiguration config, String offer,
                SessionBase.NegotiationCallback callback) {
            mAdaptee.startSession(config, offer, callback);
        }

        @Override
        public void renegotiate(
                String sessionId, String offer, SessionBase.NegotiationCallback callback) {
            mAdaptee.renegotiate(offer, callback);
        }

        @Override
        public void iceExchange(
                String sessionId, List<String> clientCandidates,
                SessionBase.IceExchangeCallback callback) {
            mAdaptee.iceExchange(clientCandidates, callback);
        }
    }
}
