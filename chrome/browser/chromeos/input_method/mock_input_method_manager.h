// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MOCK_INPUT_METHOD_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MOCK_INPUT_METHOD_MANAGER_H_

#include "chrome/browser/chromeos/input_method/input_method_util.h"
#include "ui/base/ime/chromeos/component_extension_ime_manager.h"
#include "ui/base/ime/chromeos/fake_ime_keyboard.h"
#include "ui/base/ime/chromeos/fake_input_method_delegate.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/base/ime/chromeos/input_method_whitelist.h"

namespace chromeos {
namespace input_method {

// The mock implementation of InputMethodManager for testing.
class MockInputMethodManager : public InputMethodManager {
 public:
  class State : public InputMethodManager::State {
   public:
    explicit State(MockInputMethodManager* manager);

    virtual scoped_refptr<InputMethodManager::State> Clone() const override;
    virtual void AddInputMethodExtension(
        const std::string& extension_id,
        const InputMethodDescriptors& descriptors,
        InputMethodEngineInterface* instance) override;
    virtual void RemoveInputMethodExtension(
        const std::string& extension_id) override;
    virtual void ChangeInputMethod(const std::string& input_method_id,
                                   bool show_message) override;
    virtual bool EnableInputMethod(
        const std::string& new_active_input_method_id) override;
    virtual void EnableLoginLayouts(
        const std::string& language_code,
        const std::vector<std::string>& initial_layouts) override;
    virtual void EnableLockScreenLayouts() override;
    virtual void GetInputMethodExtensions(
        InputMethodDescriptors* result) override;
    virtual scoped_ptr<InputMethodDescriptors> GetActiveInputMethods()
        const override;
    virtual const std::vector<std::string>& GetActiveInputMethodIds()
        const override;
    virtual const InputMethodDescriptor* GetInputMethodFromId(
        const std::string& input_method_id) const override;
    virtual size_t GetNumActiveInputMethods() const override;
    virtual void SetEnabledExtensionImes(
        std::vector<std::string>* ids) override;
    virtual void SetInputMethodLoginDefault() override;
    virtual void SetInputMethodLoginDefaultFromVPD(
        const std::string& locale,
        const std::string& layout) override;
    virtual bool SwitchToNextInputMethod() override;
    virtual bool SwitchToPreviousInputMethod(
        const ui::Accelerator& accelerator) override;
    virtual bool SwitchInputMethod(const ui::Accelerator& accelerator) override;
    virtual InputMethodDescriptor GetCurrentInputMethod() const override;
    virtual bool ReplaceEnabledInputMethods(
        const std::vector<std::string>& new_active_input_method_ids) override;

    // The value GetCurrentInputMethod().id() will return.
    std::string current_input_method_id;

    // The active input method ids cache (actually default only)
    std::vector<std::string> active_input_method_ids;

   protected:
    friend base::RefCounted<chromeos::input_method::InputMethodManager::State>;
    virtual ~State();

    MockInputMethodManager* const manager_;
  };

  MockInputMethodManager();
  virtual ~MockInputMethodManager();

  // InputMethodManager override:
  virtual UISessionState GetUISessionState() override;
  virtual void AddObserver(InputMethodManager::Observer* observer) override;
  virtual void AddCandidateWindowObserver(
      InputMethodManager::CandidateWindowObserver* observer) override;
  virtual void RemoveObserver(InputMethodManager::Observer* observer) override;
  virtual void RemoveCandidateWindowObserver(
      InputMethodManager::CandidateWindowObserver* observer) override;
  virtual scoped_ptr<InputMethodDescriptors>
      GetSupportedInputMethods() const override;
  virtual void ActivateInputMethodMenuItem(const std::string& key) override;
  virtual bool IsISOLevel5ShiftUsedByCurrentInputMethod() const override;
  virtual bool IsAltGrUsedByCurrentInputMethod() const override;
  virtual ImeKeyboard* GetImeKeyboard() override;
  virtual InputMethodUtil* GetInputMethodUtil() override;
  virtual ComponentExtensionIMEManager*
      GetComponentExtensionIMEManager() override;
  virtual bool IsLoginKeyboard(const std::string& layout) const override;
  virtual bool MigrateInputMethods(
       std::vector<std::string>* input_method_ids) override;
  virtual scoped_refptr<InputMethodManager::State> CreateNewState(
      Profile* profile) override;
  virtual scoped_refptr<InputMethodManager::State> GetActiveIMEState() override;
  virtual void SetState(
      scoped_refptr<InputMethodManager::State> state) override;

  // Sets an input method ID which will be returned by GetCurrentInputMethod().
  void SetCurrentInputMethodId(const std::string& input_method_id);

  void SetComponentExtensionIMEManager(
      scoped_ptr<ComponentExtensionIMEManager> comp_ime_manager);

  // Set values that will be provided to the InputMethodUtil.
  void set_application_locale(const std::string& value);

  // Set the value returned by IsISOLevel5ShiftUsedByCurrentInputMethod
  void set_mod3_used(bool value) { mod3_used_ = value; }

  // TODO(yusukes): Add more variables for counting the numbers of the API calls
  int add_observer_count_;
  int remove_observer_count_;

 protected:
  scoped_refptr<State> state_;

 private:
  FakeInputMethodDelegate delegate_;  // used by util_
  InputMethodUtil util_;
  FakeImeKeyboard keyboard_;
  bool mod3_used_;
  scoped_ptr<ComponentExtensionIMEManager> comp_ime_manager_;

  DISALLOW_COPY_AND_ASSIGN(MockInputMethodManager);
};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MOCK_INPUT_METHOD_MANAGER_H_
