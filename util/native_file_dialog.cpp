#include "util/native_file_dialog.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>  // IFileOpenDialog, COMDLG_FILTERSPEC
#include <shlobj.h>    // SHCreateItemFromParsingName
#include <wrl/client.h>  // Microsoft::WRL::ComPtr

namespace tuix {

using Microsoft::WRL::ComPtr;

namespace {

std::string to_utf8(const wchar_t* w) {
    if (!w) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string s(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);
    return s;
}

std::wstring to_utf16(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), len);
    return w;
}

}  // namespace

OpenFileResult native_open_file(const std::string& initial_dir) {
    OpenFileResult out;  // defaults to Unavailable

    // Shell dialogs want an apartment-threaded COM context. If COM is already
    // initialized on this thread in a different mode, CoInitializeEx returns
    // RPC_E_CHANGED_MODE — that's fine, we just skip the matching uninit.
    HRESULT hr_init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool did_init = SUCCEEDED(hr_init);
    if (!did_init && hr_init != RPC_E_CHANGED_MODE) return out;

    ComPtr<IFileOpenDialog> dlg;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&dlg));
    if (SUCCEEDED(hr) && dlg) {
        const COMDLG_FILTERSPEC filters[] = {
            { L"Spreadsheets (*.csv;*.xlsx)", L"*.csv;*.xlsx" },
            { L"CSV files (*.csv)",           L"*.csv" },
            { L"Excel files (*.xlsx)",        L"*.xlsx" },
            { L"All files (*.*)",             L"*.*" },
        };
        dlg->SetFileTypes(ARRAYSIZE(filters), filters);
        dlg->SetTitle(L"Open — tuix");

        if (!initial_dir.empty()) {
            const std::wstring wdir = to_utf16(initial_dir);
            ComPtr<IShellItem> folder;
            if (SUCCEEDED(SHCreateItemFromParsingName(wdir.c_str(), nullptr, IID_PPV_ARGS(&folder)))
                && folder) {
                dlg->SetFolder(folder.Get());
            }
        }

        hr = dlg->Show(nullptr);  // modal; runs its own message pump
        if (SUCCEEDED(hr)) {
            ComPtr<IShellItem> item;
            if (SUCCEEDED(dlg->GetResult(&item)) && item) {
                PWSTR psz = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
                    out.status = FileDialogStatus::Ok;
                    out.path   = to_utf8(psz);
                    CoTaskMemFree(psz);
                }
            }
        } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            out.status = FileDialogStatus::Cancelled;
        }
    }

    if (did_init) CoUninitialize();
    return out;
}

}  // namespace tuix

#elif defined(__APPLE__)  // ---- macOS: Finder "Open" panel via osascript -----

#include <array>
#include <cstdio>
#include <sys/wait.h>

namespace tuix {

namespace {

// Escape a string for embedding inside an AppleScript double-quoted literal.
std::string escape_applescript(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// Escape a string for embedding inside a single-quoted shell word.
std::string escape_shell_single(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out;
}

}  // namespace

OpenFileResult native_open_file(const std::string& initial_dir) {
    OpenFileResult out;  // defaults to Unavailable

    // AppleScript: show Finder's native Open panel, restricted to the formats
    // tuix understands, and hand back the chosen POSIX path on stdout.
    std::string script =
        "POSIX path of (choose file with prompt \"Open \xE2\x80\x94 tuix\"";
    if (!initial_dir.empty()) {
        script += " default location (POSIX file \"";
        script += escape_applescript(initial_dir);
        script += "\")";
    }
    script += " of type {\"csv\", \"xlsx\"})";

    const std::string cmd =
        "osascript -e '" + escape_shell_single(script) + "' 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return out;  // couldn't spawn — fall back to the typed dialog

    std::string result;
    std::array<char, 512> buf;
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), pipe)) > 0)
        result.append(buf.data(), n);

    const int status = pclose(pipe);

    // Trim the trailing newline osascript prints after the path.
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && !result.empty()) {
        out.status = FileDialogStatus::Ok;
        out.path   = std::move(result);
    } else {
        // Non-zero exit is overwhelmingly "user pressed Cancel" (AppleScript
        // error -128). Treat it as such; the current sheet is left untouched.
        out.status = FileDialogStatus::Cancelled;
    }
    return out;
}

}  // namespace tuix

#else  // ---- other platforms: no native picker, caller falls back ----------

namespace tuix {
OpenFileResult native_open_file(const std::string&) { return {}; }
}  // namespace tuix

#endif
