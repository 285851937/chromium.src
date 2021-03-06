// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Promise of the volume list.
 * @type {Promise}
 */
var volumeListPromise = new Promise(function(fulfill, reject) {
  chrome.fileManagerPrivate.getVolumeMetadataList(fulfill);
});

/**
 * Obtains file system of the volume type.
 * @param {string} volumeType VolumeType.
 * @return {Promise} Promise to be fulfilled with a file system.
 */
function getFileSystem(volumeType) {
  return volumeListPromise.then(function(list) {
    for (var i = 0; i < list.length; i++) {
      if (list[i].volumeType == volumeType) {
        return new Promise(function(fulfill) {
          chrome.fileManagerPrivate.requestFileSystem(
              list[i].volumeId, fulfill);
        });
      }
    }
    throw new Error('The volume is not found: ' + volumeType + '.');
  });
}

/**
 * Prepares a file on the file system.
 * @param {FileSystem} filesystem File system.
 * @param {string} name Name of the file.
 * @param {Blob} contents Contents of the file.
 * @return {Promise} Promise to be fulfilled with FileEntry of the new file.
 */
function prepareFile(filesystem, name, contents) {
  return new Promise(function(fulfill, reject) {
    filesystem.root.getFile(name, {create: true}, function(file) {
      file.createWriter(function(writer) {
        writer.write(contents);
        writer.onwrite = fulfill.bind(null, file);
        writer.onerror = reject;
      });
      fulfill(file);
    }, reject);
  });
}

/**
 * Prepares two test files on the file system.
 * @param {FileSystem} filesystem File system.
 * @return {Promise} Promise to be fullfilled with an object {filesystem:
 *     FileSystem, entries: Array.<FileEntry>} that contains the passed file
 *     system and the created entries.
 */
function prepareFiles(filesystem) {
  var testFileA =
      prepareFile(filesystem, 'test_file_a.txt', TEST_FILE_CONTENTS);
  var testFileB =
      prepareFile(filesystem, 'test_file_b.txt', TEST_FILE_CONTENTS);
  return Promise.all([testFileA, testFileB]).then(function(entries) {
    return {filesystem: filesystem, entries: entries};
  });
}

/**
 * Contents of the test file.
 * @type {Blob}
 * @const
 */
var TEST_FILE_CONTENTS = new Blob(['This is a test file.']);

/**
 * File system of the drive volume.
 * @type {Promise}
 */
var driveFileSystemPromise = getFileSystem('drive').then(prepareFiles);

/**
 * File system of the local volume.
 * @type {Promise}
 */
var localFileSystemPromise = getFileSystem('testing').then(prepareFiles);

/**
 * Calls test functions depends on the result of the promise.
 * @param {Promise} promise Promise to be fulfilled or to be rejected depends on
 *     the test results.
 */
function testPromise(promise) {
  promise.then(
      chrome.test.callbackPass(),
      function(error) {
        chrome.test.fail(error.stack || error);
      });
}

/**
 * Calls the executeTask API with the entries and checks the launch data passed
 * to onLaunched events.
 * @param {entries} entries Entries to be tested.
 * @return {Promise} Promise to be fulfilled on success.
 */
function launchWithEntries(entries) {
  var urls = entries.map(function(entry) { return entry.toURL(); });
  var tasksPromise = new Promise(function(fulfill) {
    chrome.fileManagerPrivate.getFileTasks(urls, fulfill);
  }).then(function(tasks) {
    chrome.test.assertEq(1, tasks.length);
    chrome.test.assertEq('kidcpjlbjdmcnmccjhjdckhbngnhnepk|app|textAction',
                         tasks[0].taskId);
    return tasks[0];
  });
  var launchDataPromise = new Promise(function(fulfill) {
    chrome.app.runtime.onLaunched.addListener(function handler(launchData) {
      chrome.app.runtime.onLaunched.removeListener(handler);
      fulfill(launchData);
    });
  });
  var taskExecutedPromise = tasksPromise.then(function(task) {
    return new Promise(function(fulfill, reject) {
      chrome.fileManagerPrivate.executeTask(
          task.taskId,
          urls,
          function(result) {
            if (result)
              fulfill();
            else
              reject();
          });
      });
  });
  var resolvedEntriesPromise = launchDataPromise.then(function(launchData) {
    var entries = launchData.items.map(function(item) { return item.entry; });
    return new Promise(function(fulfill) {
      chrome.fileManagerPrivate.resolveIsolatedEntries(entries, fulfill);
    });
  });
  return Promise.all([
    taskExecutedPromise,
    launchDataPromise,
    resolvedEntriesPromise
  ]).then(function(args) {
    chrome.test.assertEq(entries.length, args[1].items.length);
    chrome.test.assertEq(
        entries.map(function(entry) { return entry.name; }),
        args[1].items.map(function(item) { return item.entry.name; }),
        'Wrong entries are passed to the application handler.');
    chrome.test.assertEq(
        entries.map(function(entry) { return entry.toURL(); }),
        args[2].map(function(entry) { return entry.toURL(); }),
        'Entries passed to the application handler cannot be resolved.');
  });
}

/**
 * Tests the file handler feature with entries on the local volume.
 */
function testForLocalFiles() {
  testPromise(localFileSystemPromise.then(function(volume) {
    return launchWithEntries(volume.entries);
  }));
}

/**
 * Tests the file handler feature with entries on the drive volume.
 */
function testForDriveFiles() {
  testPromise(driveFileSystemPromise.then(function(volume) {
    return launchWithEntries(volume.entries);
  }));
}

/**
 * Tests the file handler with entries both on the local and on the drive
 * volumes.
 */
function testForMixedFiles() {
  testPromise(
      Promise.all([localFileSystemPromise, driveFileSystemPromise]).then(
          function(args) {
            return launchWithEntries(args[0].entries.concat(args[1].entries));
          }));
}

// Run the tests.
chrome.test.runTests([
  testForLocalFiles,
  testForDriveFiles,
  testForMixedFiles
]);
