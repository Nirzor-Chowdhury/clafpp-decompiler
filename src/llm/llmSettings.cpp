// llmSettings.cpp
// ----------------------------------------------------------------------------
// wxWidgets settings dialog implementation.
// ----------------------------------------------------------------------------

#include "llmSettings.h"

#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/textdlg.h>

#include <thread>

namespace jdc {
namespace llm {

wxBEGIN_EVENT_TABLE(LLMSettingsDialog, wxFrame)
    EVT_BUTTON(LLMSettingsDialog::RefreshModelsButtonID,  LLMSettingsDialog::OnRefreshModels)
    EVT_BUTTON(LLMSettingsDialog::TestConnectionButtonID, LLMSettingsDialog::OnTestConnection)
    EVT_BUTTON(LLMSettingsDialog::PullModelButtonID,      LLMSettingsDialog::OnPullModel)
    EVT_BUTTON(LLMSettingsDialog::SaveButtonID,           LLMSettingsDialog::OnSave)
    EVT_BUTTON(LLMSettingsDialog::CancelButtonID,         LLMSettingsDialog::OnCancel)
    EVT_CLOSE(LLMSettingsDialog::OnClose)
wxEND_EVENT_TABLE()

LLMSettingsDialog::LLMSettingsDialog(wxWindow* parent,
                                     Settings initial,
                                     std::string configPath,
                                     SettingsChangedCallback onChanged)
    : wxFrame(parent, MainWindowID, "LLM Settings",
              wxPoint(50, 50), wxSize(500, 600))
    , m_settings(std::move(initial))
    , m_configPath(std::move(configPath))
    , m_onChanged(std::move(onChanged))
    , m_probe(std::make_unique<LLMCommentGenerator>(m_settings))
{
    SetOwnBackgroundColour(backgroundColor);
    BuildUi();
    LoadFromSettings(m_settings);
}

void LLMSettingsDialog::BuildUi()
{
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto makeLabel = [this](const wxString& s)
    {
        auto* l = new wxStaticText(this, wxID_ANY, s);
        l->SetOwnForegroundColour(textColor);
        return l;
    };

    auto addRow = [&](const wxString& label, wxWindow* ctrl)
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = makeLabel(label);
        row->Add(lbl,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        row->Add(ctrl, 1, wxEXPAND);
        root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    };

    // --- Endpoint --------------------------------------------------------
    m_endpointText = new wxTextCtrl(this, EndpointTextID, "",
                                    wxDefaultPosition, wxSize(300, -1));
    m_endpointText->SetOwnBackgroundColour(foregroundColor);
    m_endpointText->SetOwnForegroundColour(textColor);
    addRow("Endpoint:", m_endpointText);

    // --- Model + Refresh -------------------------------------------------
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = makeLabel("Model:");
        m_modelChoice = new wxChoice(this, ModelChoiceID,
                                     wxDefaultPosition, wxSize(220, -1));
        m_modelChoice->SetOwnBackgroundColour(foregroundColor);
        m_modelChoice->SetOwnForegroundColour(textColor);

        m_refreshBtn = new wxButton(this, RefreshModelsButtonID, "Refresh",
                                    wxDefaultPosition, wxSize(80, -1));
        m_refreshBtn->SetOwnBackgroundColour(foregroundColor);
        m_refreshBtn->SetOwnForegroundColour(textColor);

        row->Add(lbl,            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        row->Add(m_modelChoice,  1, wxEXPAND | wxRIGHT, 10);
        row->Add(m_refreshBtn,   0);
        root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    }

    // --- Temperature -----------------------------------------------------
    m_temperatureSpin = new wxSpinCtrlDouble(this, TemperatureSpinID, "",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxSP_ARROW_KEYS, 0.0, 2.0, 0.7, 0.05);
    addRow("Temperature:", m_temperatureSpin);

    m_topPSpin = new wxSpinCtrlDouble(this, TopPSpinID, "",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 0.0, 1.0, 0.9, 0.05);
    addRow("Top-p:", m_topPSpin);

    m_maxTokensSpin = new wxSpinCtrl(this, MaxTokensSpinID, "",
                                     wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 32, 4096, 500);
    addRow("Max tokens:", m_maxTokensSpin);

    m_timeoutSpin = new wxSpinCtrl(this, TimeoutSpinID, "",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, 5, 1800, 120);
    addRow("Timeout (s):", m_timeoutSpin);

    // --- Comment style ---------------------------------------------------
    m_commentStyleChoice = new wxChoice(this, CommentStyleChoiceID);
    m_commentStyleChoice->SetOwnBackgroundColour(foregroundColor);
    m_commentStyleChoice->SetOwnForegroundColour(textColor);
    m_commentStyleChoice->Append("Simple");
    m_commentStyleChoice->Append("Doxygen");
    m_commentStyleChoice->Append("Verbose");
    addRow("Comment style:", m_commentStyleChoice);

    // --- Checkboxes ------------------------------------------------------
    auto makeCheck = [this](int id, const wxString& s)
    {
        auto* c = new wxCheckBox(this, id, s);
        c->SetOwnForegroundColour(textColor);
        c->SetOwnBackgroundColour(backgroundColor);
        return c;
    };
    m_cacheCheck         = makeCheck(CacheCheckID,            "Cache results in memory");
    m_autoDetectCheck    = makeCheck(AutoDetectCheckID,       "Auto-pull missing models");
    m_funcCommentsCheck  = makeCheck(FunctionCommentsCheckID, "Generate function comments by default");
    m_lineCommentsCheck  = makeCheck(LineCommentsCheckID,     "Generate line comments by default");
    root->Add(m_cacheCheck,         0, wxLEFT | wxTOP, 10);
    root->Add(m_autoDetectCheck,    0, wxLEFT | wxTOP, 6);
    root->Add(m_funcCommentsCheck,  0, wxLEFT | wxTOP, 6);
    root->Add(m_lineCommentsCheck,  0, wxLEFT | wxTOP, 6);

    // --- Action buttons --------------------------------------------------
    auto makeBtn = [this](int id, const wxString& s)
    {
        auto* b = new wxButton(this, id, s, wxDefaultPosition, wxSize(110, -1));
        b->SetOwnBackgroundColour(foregroundColor);
        b->SetOwnForegroundColour(textColor);
        return b;
    };

    auto* actionRow = new wxBoxSizer(wxHORIZONTAL);
    m_testBtn   = makeBtn(TestConnectionButtonID, "Test connection");
    m_pullBtn   = makeBtn(PullModelButtonID,      "Pull model...");
    actionRow->Add(m_testBtn, 0, wxRIGHT, 10);
    actionRow->Add(m_pullBtn, 0);
    root->Add(actionRow, 0, wxLEFT | wxTOP, 10);

    // --- Status line -----------------------------------------------------
    m_statusLabel = makeLabel("");
    root->Add(m_statusLabel, 0, wxALL, 10);

    // --- Save / Cancel ---------------------------------------------------
    auto* bottomRow = new wxBoxSizer(wxHORIZONTAL);
    m_saveBtn   = makeBtn(SaveButtonID,   "Save");
    m_cancelBtn = makeBtn(CancelButtonID, "Cancel");
    bottomRow->AddStretchSpacer();
    bottomRow->Add(m_cancelBtn, 0, wxRIGHT, 10);
    bottomRow->Add(m_saveBtn,   0);
    root->Add(bottomRow, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(root);
}

void LLMSettingsDialog::LoadFromSettings(const Settings& s)
{
    m_endpointText->SetValue(s.ollama.endpoint);
    m_temperatureSpin->SetValue(s.ollama.temperature);
    m_topPSpin->SetValue(s.ollama.topP);
    m_maxTokensSpin->SetValue(s.ollama.maxTokens);
    m_timeoutSpin->SetValue(s.ollama.timeoutSec);

    switch (s.commenting.commentStyle)
    {
        case CommentStyle::Doxygen: m_commentStyleChoice->SetSelection(1); break;
        case CommentStyle::Verbose: m_commentStyleChoice->SetSelection(2); break;
        case CommentStyle::Simple:
        default:                    m_commentStyleChoice->SetSelection(0); break;
    }

    m_cacheCheck       ->SetValue(s.commenting.cacheResults);
    m_autoDetectCheck  ->SetValue(s.commenting.autoDetectModels);
    m_funcCommentsCheck->SetValue(s.commenting.generateFunctionComments);
    m_lineCommentsCheck->SetValue(s.commenting.generateLineComments);

    // Populate model dropdown with at least the configured name so the user
    // always sees something. The Refresh button reaches out to Ollama.
    m_modelChoice->Clear();
    m_modelChoice->Append(s.ollama.modelName);
    m_modelChoice->SetSelection(0);
}

Settings LLMSettingsDialog::BuildSettingsFromUi() const
{
    Settings s = m_settings;
    s.ollama.endpoint    = std::string(m_endpointText->GetValue().mb_str());
    s.ollama.temperature = m_temperatureSpin->GetValue();
    s.ollama.topP        = m_topPSpin->GetValue();
    s.ollama.maxTokens   = m_maxTokensSpin->GetValue();
    s.ollama.timeoutSec  = m_timeoutSpin->GetValue();

    int sel = m_modelChoice->GetSelection();
    if (sel != wxNOT_FOUND)
        s.ollama.modelName = std::string(m_modelChoice->GetString(sel).mb_str());

    int styleSel = m_commentStyleChoice->GetSelection();
    s.commenting.commentStyle = (styleSel == 1) ? CommentStyle::Doxygen
                              : (styleSel == 2) ? CommentStyle::Verbose
                              :                   CommentStyle::Simple;

    s.commenting.cacheResults             = m_cacheCheck->GetValue();
    s.commenting.autoDetectModels         = m_autoDetectCheck->GetValue();
    s.commenting.generateFunctionComments = m_funcCommentsCheck->GetValue();
    s.commenting.generateLineComments     = m_lineCommentsCheck->GetValue();
    return s;
}

void LLMSettingsDialog::SetStatus(const wxString& msg, bool isError)
{
    if (!m_statusLabel) return;
    m_statusLabel->SetForegroundColour(isError ? wxColour(250, 80, 80) : textColor);
    m_statusLabel->SetLabel(msg);
    Layout();
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void LLMSettingsDialog::OnRefreshModels(wxCommandEvent&)
{
    Settings ui = BuildSettingsFromUi();
    m_probe->updateConfig(ui);
    SetStatus("Connecting to Ollama...");
    m_refreshBtn->Disable();

    // Run the network call on a worker thread so the UI stays responsive.
    std::thread([this]()
    {
        std::vector<std::string> models;
        bool ok = m_probe->validateOllamaConnection();
        if (ok) models = m_probe->getAvailableModels();
        std::string err = m_probe->getLastError();
        std::string currentSel = std::string(m_modelChoice->GetString(
            std::max(0, m_modelChoice->GetSelection())).mb_str());

        CallAfter([this, ok, models, err, currentSel]()
        {
            if (!ok)
            {
                SetStatus("Cannot reach Ollama: " + wxString(err), /*err*/true);
                m_refreshBtn->Enable();
                return;
            }
            m_modelChoice->Clear();
            if (models.empty())
            {
                m_modelChoice->Append("(no models installed)");
                m_modelChoice->SetSelection(0);
                SetStatus("Connected to Ollama but no models are installed. "
                          "Use 'Pull model...' to download one.", /*err*/true);
            }
            else
            {
                int wantedIdx = -1;
                for (size_t i = 0; i < models.size(); ++i)
                {
                    m_modelChoice->Append(models[i]);
                    if (wantedIdx < 0 &&
                        (models[i] == currentSel ||
                         models[i].rfind(currentSel + ":", 0) == 0))
                        wantedIdx = static_cast<int>(i);
                }
                m_modelChoice->SetSelection(wantedIdx >= 0 ? wantedIdx : 0);
                SetStatus(wxString::Format("Found %zu model(s).", models.size()));
            }
            m_refreshBtn->Enable();
        });
    }).detach();
}

void LLMSettingsDialog::OnTestConnection(wxCommandEvent&)
{
    Settings ui = BuildSettingsFromUi();
    m_probe->updateConfig(ui);
    SetStatus("Testing connection and selected model...");
    m_testBtn->Disable();

    std::thread([this]()
    {
        bool reachable = m_probe->validateOllamaConnection();
        std::string err = m_probe->getLastError();

        std::string sampleReply;
        if (reachable)
        {
            GenerateOptions opts;
            opts.maxTokens   = 16;
            opts.temperature = 0.0;
            opts.timeoutSec  = 30;
            sampleReply = m_probe->getConfig().ollama.modelName;
            // We send a 1-token-ish probe just to verify the model actually
            // generates. This may pull-on-demand on Ollama's side, hence the
            // 30s timeout.
            OllamaInterface tmp(m_probe->getConfig().ollama.endpoint);
            std::string r = tmp.makeRequest(sampleReply, "Reply with exactly the word: OK", 4, 0.0);
            sampleReply = r;
            if (r.empty()) { reachable = false; err = tmp.getLastError(); }
        }

        CallAfter([this, reachable, err, sampleReply]()
        {
            if (reachable)
                SetStatus("Connection OK. Model replied: " +
                          wxString(sampleReply).Trim().Trim(false));
            else
                SetStatus("Test failed: " + wxString(err), /*err*/true);
            m_testBtn->Enable();
        });
    }).detach();
}

void LLMSettingsDialog::OnPullModel(wxCommandEvent&)
{
    wxTextEntryDialog dlg(this,
        "Enter the Ollama model name to pull (e.g. mistral, neural-chat, "
        "llama2, dolphin-mixtral):", "Pull model",
        m_modelChoice->GetSelection() == wxNOT_FOUND ? wxString("mistral")
            : m_modelChoice->GetString(m_modelChoice->GetSelection()));
    if (dlg.ShowModal() != wxID_OK) return;
    std::string modelName(dlg.GetValue().mb_str());
    if (modelName.empty()) return;

    Settings ui = BuildSettingsFromUi();
    m_probe->updateConfig(ui);

    // wxProgressDialog runs modal; we drive the pull on a worker thread and
    // pump updates through CallAfter. The dialog supports a Cancel button.
    auto* prog = new wxProgressDialog(
        "Pulling model",
        wxString::Format("Pulling '%s' from Ollama...", modelName),
        100, this,
        wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_ELAPSED_TIME);

    auto pullWorker = [this, modelName, prog]()
    {
        bool aborted = false;
        // Reach the OllamaInterface inside the generator via a fresh one,
        // so we can call cancel() independently if the user hits Cancel.
        OllamaInterface puller(m_probe->getConfig().ollama.endpoint);

        // Periodically poll prog->Update from a watcher thread? Simpler: the
        // pull callback runs on the worker thread, so we just call CallAfter
        // for UI updates. We use a small atomic to signal Cancel.
        std::atomic<bool> cancelled(false);
        std::atomic<bool>* pCancelled = &cancelled;
        std::thread watcher([this, pCancelled, &puller, prog]()
        {
            // Poll the progress dialog's Cancel flag on the UI thread.
            while (!pCancelled->load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                std::atomic<bool> done(false);
                std::atomic<bool>* pDone = &done;
                CallAfter([prog, pDone, pCancelled]()
                {
                    if (prog->WasCancelled()) pCancelled->store(true);
                    pDone->store(true);
                });
                while (!done.load() && !pCancelled->load())
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            puller.cancel();
        });

        bool ok = puller.pullModel(modelName, [this, prog, pCancelled](const PullProgress& p)
        {
            int pct = (p.total > 0)
                ? static_cast<int>((p.completed * 100) / p.total)
                : -1;
            wxString status(p.status);
            CallAfter([prog, pct, status]()
            {
                if (pct >= 0 && pct <= 100)
                    prog->Update(pct, status);
                else
                    prog->Pulse(status);
            });
        });
        std::string err = puller.getLastError();
        aborted = !ok && (puller.isCancelled() || cancelled.load());

        cancelled = true;
        if (watcher.joinable()) watcher.join();

        CallAfter([this, prog, ok, aborted, err, modelName]()
        {
            prog->Update(100, "Done");
            prog->Destroy();
            if (ok)
            {
                SetStatus(wxString::Format("Pulled '%s' successfully.", modelName));
                // Refresh the dropdown
                wxCommandEvent dummy;
                OnRefreshModels(dummy);
            }
            else if (aborted)
            {
                SetStatus("Pull cancelled.", /*err*/true);
            }
            else
            {
                SetStatus("Pull failed: " + wxString(err), /*err*/true);
            }
        });
    };
    std::thread(std::move(pullWorker)).detach();
}

void LLMSettingsDialog::OnSave(wxCommandEvent&)
{
    m_settings = BuildSettingsFromUi();
    std::string err;
    if (!Settings::save(m_settings, m_configPath, &err))
    {
        wxMessageBox("Failed to save settings:\n" + wxString(err),
                     "Save error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (m_onChanged) m_onChanged(m_settings);
    Hide();
}

void LLMSettingsDialog::OnCancel(wxCommandEvent&)
{
    Hide();
}

void LLMSettingsDialog::OnClose(wxCloseEvent& /*e*/)
{
    // Mirror ColorsMenu: hide rather than destroy so state survives.
    Hide();
}

void LLMSettingsDialog::OpenMenu(wxPoint position)
{
    position.x += 10;
    position.y += 10;
    SetPosition(position);
    Show();
    Raise();
}

} // namespace llm
} // namespace jdc
