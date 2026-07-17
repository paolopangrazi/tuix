#include "util/native_file_dialog.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>  // IFileOpenDialog, COMDLG_FILTERSPEC
#include <shlobj.h>    // SHCreateItemFromParsingName

namespace tuix {

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

    IFileOpenDialog* dlg = nullptr;
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
            IShellItem* folder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(wdir.c_str(), nullptr, IID_PPV_ARGS(&folder)))
                && folder) {
                dlg->SetFolder(folder);
                folder->Release();
            }
        }

        hr = dlg->Show(nullptr);  // modal; runs its own message pump
        if (SUCCEEDED(hr)) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dlg->GetResult(&item)) && item) {
                PWSTR psz = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
                    out.status = FileDialogStatus::Ok;
                    out.path   = to_utf8(psz);
                    CoTaskMemFree(psz);
                }
                item->Release();
            }
        } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            out.status = FileDialogStatus::Cancelled;
        }
        dlg->Release();
    }

    if (did_init) CoUninitialize();
    return out;
}

}  // namespace tuix

#else  // ---- non-Windows: no native picker, caller falls back --------------

namespace tuix {
OpenFileResult native_open_file(const std::string&) { return {}; }
}  // namespace tuix

#endif
