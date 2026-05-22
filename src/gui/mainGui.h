#pragma once
#include "guiUtils.h"
#include <wx/grid.h>
#include "disassembleBytesMenu.h"
#include "dataViewerMenu.h"
#include "importsViewerMenu.h"
#include "colorsMenu.h"
#include "functionInfoMenu.h"
#include "../file-handler/fileHandler.h"
#include "../disassembler/disassembler.h"
#include "../decompiler/decompiler.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace jdc { namespace llm {
    class LLMCommentGenerator;
    class LLMSettingsDialog;
    struct Settings;
}}

class FunctionPropertiesMenu;

class MainGui : public wxFrame, public Utils
{
public:
	MainGui();

	wxMenuBar* menuBar = nullptr;
	BytesDisassembler* bytesDisassemblerMenu = nullptr;
	DataViewer* dataViewerMenu = nullptr;
	ImportsViewer* importsViewerMenu = nullptr;
	ColorsMenu* colorsMenu = nullptr;
	FunctionPropertiesMenu* functionPropertiesMenu = nullptr;
	FunctionInfoMenu* functionInfoMenu = nullptr;

	// LLM members
	std::unique_ptr<jdc::llm::LLMCommentGenerator> llmGenerator;
	jdc::llm::LLMSettingsDialog* llmSettingsDialog = nullptr;
	std::thread       llmWorkerThread;
	std::atomic<bool> llmBusy{false};

	wxStaticText* statusStaticText = nullptr;

	wxStyledTextCtrl* disassemblyTextCtrl = nullptr;
	wxStyledTextCtrl* decompilationTextCtrl = nullptr;

	wxGrid* functionsGrid = nullptr;

	wxBoxSizer* row1Sizer = nullptr;
	wxBoxSizer* row2Sizer = nullptr;
	wxBoxSizer* vSizer = nullptr;

	wxString currentFilePath = "";
	unsigned char is64Bit = 0;
	unsigned long long imageBase = 0;

	ImportedFunction* imports = nullptr;
	int numOfImports = 0;

	std::vector<DisassembledInstruction> disassembledInstructions;

	std::vector<Function> functions;

	unsigned char* dataSectionBytes = nullptr;
	static const int dataSectionsBufferSize = 10;
	FileSection dataSections[dataSectionsBufferSize] = { 0 };
	int numOfDataSections = 0;

	static const int codeSectionsBufferSize = 10;
	FileSection codeSections[codeSectionsBufferSize] = { 0 };
	int numOfCodeSections = 0;

	int currentDecompiledFunc = -1;

	DecompilationParameters decompParams = { 0 };
	
	enum ids 
	{
		MainWindowID,
		OpenBytesDisassemblerID,
		OpenDataViewerID,
		OpenImportsViewerID,
		OpenColorsMenuID,
		OpenFileID,
		DisassembleFileID,
		AnalyzeFileID,
		DisassembleFileButtonID,
		AnalyzeFileButtonID,

		// LLM menu IDs
		LLMGenerateFunctionCommentID,
		LLMGenerateLineCommentsID,
		LLMSuggestVariableNamesID,
		LLMAnalyzeComplexityID,
		LLMOpenSettingsID
	};

	void OpenFile();

	void DisassembleFile();

	void AnalyzeFile();

	void ClearData();

	void LoadDataSectionBytes();

	void DisassembleCodeSections();

	void DecompileFunction(int functionIndex);

	void FindAllFunctions();

	void UpdateDisassemblyTextCtrl();

	void UpdateFunctionsGrid();

	void GetFunctionSymbols();

	void GridRightClickOptions(wxGridEvent& e);

	void StyledTextCtrlRightClickOptions(wxContextMenuEvent& e);

	void CloseApp(wxCloseEvent& e);

	char IsCharDigit(char c);

	void ApplySyntaxHighlighting(Function* function);

	void ApplyAsmHighlighting();

	void InitLLM();
	void ShutdownLLM();
	void LLMGenerateFunctionComment();
	void LLMGenerateLineComments();
	void LLMSuggestVariableNames();
	void LLMAnalyzeComplexity();
	void LLMOpenSettings();
	bool RunLLMAsync(std::function<std::string(std::string&)> task,
	                 std::function<void(const std::string&, const std::string&)> onDone,
	                 const wxString& busyMessage);
	bool GetCurrentDecompiledCode(std::string& outCode, std::string& outName);
	void InsertCommentAtTop(const std::string& comment);

	void ColorAllStrs(wxString text, const char* string, ColorsMenu::DecompilationColor color, unsigned char forceColor);

	wxDECLARE_EVENT_TABLE();
};

class FunctionPropertiesMenu : public wxFrame, public Utils
{
public:
	FunctionPropertiesMenu(wxPoint position, MainGui* main, int funcIndex);

	wxStaticText* functionNameLabel = nullptr;
	wxTextCtrl* functionNameTextCtrl;

	std::vector<wxTextCtrl*> regArgNameTextCtrls;
	std::vector<wxTextCtrl*> regVarNameTextCtrls;
	std::vector<wxTextCtrl*> stackArgNameTextCtrls;
	std::vector<wxTextCtrl*> stackVarNameTextCtrls;
	std::vector<wxTextCtrl*> retVarNameTextCtrls;

	wxBoxSizer* vSizer = nullptr;

	MainGui* mainGui = nullptr;
	int functionIndex = -1;

	enum ids
	{
		MainWindowID
	};

	void CloseMenu(wxCloseEvent& e);

	wxDECLARE_EVENT_TABLE();
};
