#pragma once

// llmSettings.h
// ----------------------------------------------------------------------------
// wxWidgets settings dialog for the local Ollama integration.
//
// The dialog mirrors the look and feel of ColorsMenu (dark theme via the
// existing Utils base class). It exposes:
//   * Ollama endpoint URL
//   * Model dropdown (auto-populated via /api/tags)
//   * Temperature, max tokens, top-p, timeout
//   * Comment style + cache toggle
//   * Buttons: Refresh models, Test connection, Pull model, Save, Cancel
//
// The dialog owns its own LLMCommentGenerator instance for connection tests
// and model listing; the MainGui retains a separate long-lived instance for
// actual comment generation. Settings written via Save() are persisted to
// the JSON config file AND propagated back to the caller via a callback.
// ----------------------------------------------------------------------------

#include "../gui/guiUtils.h"
#include "llmConfig.h"
#include "llmCommentGenerator.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include <functional>
#include <memory>
#include <string>

namespace jdc {
namespace llm {

class LLMSettingsDialog : public wxFrame, public Utils
{
public:
    using SettingsChangedCallback = std::function<void(const Settings&)>;

    LLMSettingsDialog(wxWindow* parent,
                      Settings initial,
                      std::string configPath,
                      SettingsChangedCallback onChanged);

    // Show / hide like ColorsMenu does (frame is reused, not destroyed).
    void OpenMenu(wxPoint position);

    enum ids
    {
        MainWindowID = wxID_HIGHEST + 200,
        EndpointTextID,
        ModelChoiceID,
        TemperatureSpinID,
        TopPSpinID,
        MaxTokensSpinID,
        TimeoutSpinID,
        CommentStyleChoiceID,
        CacheCheckID,
        AutoDetectCheckID,
        FunctionCommentsCheckID,
        LineCommentsCheckID,
        RefreshModelsButtonID,
        TestConnectionButtonID,
        PullModelButtonID,
        SaveButtonID,
        CancelButtonID
    };

private:
    void BuildUi();
    void LoadFromSettings(const Settings& s);
    Settings BuildSettingsFromUi() const;

    void OnRefreshModels(wxCommandEvent&);
    void OnTestConnection(wxCommandEvent&);
    void OnPullModel(wxCommandEvent&);
    void OnSave(wxCommandEvent&);
    void OnCancel(wxCommandEvent&);
    void OnClose(wxCloseEvent&);

    void SetStatus(const wxString& msg, bool isError = false);

    // ---- Widgets ----------------------------------------------------------
    wxTextCtrl*       m_endpointText        = nullptr;
    wxChoice*         m_modelChoice         = nullptr;
    wxSpinCtrlDouble* m_temperatureSpin     = nullptr;
    wxSpinCtrlDouble* m_topPSpin            = nullptr;
    wxSpinCtrl*       m_maxTokensSpin       = nullptr;
    wxSpinCtrl*       m_timeoutSpin         = nullptr;
    wxChoice*         m_commentStyleChoice  = nullptr;
    wxCheckBox*       m_cacheCheck          = nullptr;
    wxCheckBox*       m_autoDetectCheck     = nullptr;
    wxCheckBox*       m_funcCommentsCheck   = nullptr;
    wxCheckBox*       m_lineCommentsCheck   = nullptr;

    wxButton*         m_refreshBtn          = nullptr;
    wxButton*         m_testBtn             = nullptr;
    wxButton*         m_pullBtn             = nullptr;
    wxButton*         m_saveBtn             = nullptr;
    wxButton*         m_cancelBtn           = nullptr;

    wxStaticText*     m_statusLabel         = nullptr;

    // ---- State ------------------------------------------------------------
    Settings                              m_settings;
    std::string                           m_configPath;
    SettingsChangedCallback               m_onChanged;
    std::unique_ptr<LLMCommentGenerator>  m_probe; // for test/refresh only

    wxDECLARE_EVENT_TABLE();
};

} // namespace llm
} // namespace jdc
