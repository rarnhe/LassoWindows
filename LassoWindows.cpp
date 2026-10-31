#include <windows.h>
#include <windowsx.h> // 👈 添加这行
#include <shlwapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <fstream>  // 必须
#include <shlobj.h>
#include <propkey.h>
#include <propvarutil.h>
#include <filesystem>
#include <unordered_map>

// WinRT 通知相关
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
// 链接库
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "propsys.lib")
using namespace winrt;
using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;

static std::wstring cachedAppId = []() {
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	return std::filesystem::path(path).stem().wstring();
	}();

//// --- 修正后的 Shell 注册函数 ---
//HRESULT RegisterAppShortcut() {
//	wchar_t exePath[MAX_PATH];
//	GetModuleFileNameW(NULL, exePath, MAX_PATH);
//	// 转为路径对象并指向同目录下的 ini
//	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
//	std::wstring desktopIniPath = (exeDir / L"desktop.ini").wstring();
//	//wchar_t programsPath[MAX_PATH];
//	// 使用 SHGetKnownFolderPath 替代过时的 SHGetSpecialFolderPath
//	PWSTR pszPath = nullptr;
//	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Programs, 0, NULL, &pszPath);
//	if (FAILED(hr)) return hr;
//	std::wstring exeName = std::filesystem::path(exePath).stem().wstring();
//	std::wstring lnkPath = std::wstring(pszPath) + L"\\" + exeName + L".lnk";
//	CoTaskMemFree(pszPath);
//	if (PathFileExistsW(lnkPath.c_str())) return S_OK;
//	IShellLinkW* psl;
//	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
//	if (SUCCEEDED(hr)) {
//		psl->SetPath(exePath);
//		// 动态读取图标逻辑（使用局部变量 finalIconPath 和 finalIconIndex）
//		std::wstring finalIconPath = L"";
//		int finalIconIndex = 0;
//		std::ifstream iniFile(desktopIniPath, std::ios::in | std::ios::binary);
//		if (iniFile.is_open()) {
//			std::string line;
//			while (std::getline(iniFile, line)) {
//				// ANSI 转 wstring
//				int size_needed = MultiByteToWideChar(CP_ACP, 0, line.c_str(), (int)line.size(), NULL, 0);
//				std::wstring wline(size_needed, 0);
//				MultiByteToWideChar(CP_ACP, 0, line.c_str(), (int)line.size(), &wline[0], size_needed);
//				size_t pos = wline.find(L"IconResource=");
//				if (pos != std::wstring::npos) {
//					std::wstring resValue = wline.substr(pos + 13);
//					size_t commaPos = resValue.find_last_of(L',');
//					if (commaPos != std::wstring::npos) {
//						finalIconPath = resValue.substr(0, commaPos);
//						try {
//							finalIconIndex = std::stoi(resValue.substr(commaPos + 1));
//						}
//						catch (...) { finalIconIndex = 0; }
//					}
//					break;
//				}
//			}
//			iniFile.close();
//		}
//		//// --- 开始 MessageBox 验证 ---
//		//std::wstring validationMsg = L"Desktop.ini 解析结果：\n\n";
//		//validationMsg += L"文件路径: " + desktopIniPath + L"\n";
//		//validationMsg += L"解析状态: " + std::wstring(finalIconPath.empty() ? L"未找到配置" : L"解析成功") + L"\n\n";
//		//if (!finalIconPath.empty()) {
//		// validationMsg += L"图标路径 (Raw): " + finalIconPath + L"\n";
//		// validationMsg += L"图标索引: " + std::to_wstring(finalIconIndex) + L"\n";
//		// // 额外验证：展开环境变量（处理 %SystemRoot% 等情况）
//		// wchar_t expandedPath[MAX_PATH];
//		// ExpandEnvironmentStringsW(finalIconPath.c_str(), expandedPath, MAX_PATH);
//		// validationMsg += L"实际物理路径: " + std::wstring(expandedPath);
//		//}
//		//MessageBoxW(NULL, validationMsg.c_str(), L"Debug - 图标解析验证", MB_OK | MB_ICONINFORMATION);
//		//// --- 验证结束 ---
//		// 4. 应用图标设置
//		if (!finalIconPath.empty()) {
//			psl->SetIconLocation(finalIconPath.c_str(), finalIconIndex);
//		}
//		//else {
//		// // 回退方案：默认 shell32 图标
//		// psl->SetIconLocation(L"C:\\Windows\\System32\\shell32.dll", 293);
//		//}
//		IPropertyStore* pps;
//		hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
//		if (SUCCEEDED(hr)) {
//			PROPVARIANT var;
//			// 确保使用 InitPropVariantFromString (Unicode 版本)
//			hr = InitPropVariantFromString(cachedAppId.c_str(), &var);
//			if (SUCCEEDED(hr)) {
//				hr = pps->SetValue(PKEY_AppUserModel_ID, var);
//				pps->Commit();
//				PropVariantClear(&var);
//			}
//			pps->Release();
//		}
//		IPersistFile* ppf;
//		hr = psl->QueryInterface(IID_PPV_ARGS(&ppf));
//		if (SUCCEEDED(hr)) {
//			hr = ppf->Save(lnkPath.c_str(), TRUE);
//			ppf->Release();
//			// 只有保存成功后才刷新系统缓存
//			if (SUCCEEDED(hr)) {
//				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
//			}
//		}
//		psl->Release();
//	}
//	return hr;
//}


/**
 * 从指定的 desktop.ini 文件中解析图标资源
 * @param iniPath ini 文件的完整路径
 * @param outIconPath 返回解析出的图标路径（如 DLL 或 ICO 路径）
 * @param outIconIndex 返回图标索引
 * @return 是否解析成功
 */
bool GetIconResourceFromIni(std::wstring& outIconPath, int& outIconIndex) {
	outIconPath.clear();
	outIconIndex = 0;

	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	// 1. 构造桌面 ini 路径
	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	std::wstring desktopIniPath = (exeDir / L"desktop.ini").wstring();

	// 以二进制方式读取，避免不同编码带来的换行符问题
	std::ifstream iniFile(desktopIniPath, std::ios::in | std::ios::binary);
	if (!iniFile.is_open()) return false;

	std::string line;
	while (std::getline(iniFile, line)) {
		// ANSI (CP_ACP) 转 wstring，处理中文路径或特殊字符
		int size_needed = MultiByteToWideChar(CP_ACP, 0, line.c_str(), (int)line.size(), NULL, 0);
		if (size_needed <= 0) continue;

		std::wstring wline(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, line.c_str(), (int)line.size(), &wline[0], size_needed);

		// 查找 IconResource 键
		size_t pos = wline.find(L"IconResource=");
		if (pos != std::wstring::npos) {
			// 截取等号后的内容
			std::wstring resValue = wline.substr(pos + 13);

			// 图标资源格式通常为: Path,Index
			size_t commaPos = resValue.find_last_of(L',');
			if (commaPos != std::wstring::npos) {
				outIconPath = resValue.substr(0, commaPos);
				try {
					outIconIndex = std::stoi(resValue.substr(commaPos + 1));
				}
				catch (...) {
					outIconIndex = 0;
				}
			}
			else {
				// 如果没有逗号，认为整个字符串就是路径
				outIconPath = resValue;
				outIconIndex = 0;
			}
			iniFile.close();
			return !outIconPath.empty();
		}
	}
	iniFile.close();
	return false;
}

HRESULT RegisterAppShortcut() {
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	// 1. 构造桌面 ini 路径
	//std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	//std::wstring desktopIniPath = (exeDir / L"desktop.ini").wstring();

	// 2. 构造开始菜单快捷方式路径
	PWSTR pszPath = nullptr;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Programs, 0, NULL, &pszPath);
	if (FAILED(hr)) return hr;

	std::wstring exeName = std::filesystem::path(exePath).stem().wstring();
	std::wstring lnkPath = std::wstring(pszPath) + L"\\" + exeName + L".lnk";
	CoTaskMemFree(pszPath);

	// 如果快捷方式已存在则跳过
	if (PathFileExistsW(lnkPath.c_str())) return S_OK;

	// 3. 创建 ShellLink 对象
	IShellLinkW* psl;
	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
	if (SUCCEEDED(hr)) {
		psl->SetPath(exePath);

		// --- 调用独立出的解析函数 ---
		std::wstring finalIconPath;
		int finalIconIndex = 0;
		if (GetIconResourceFromIni(finalIconPath, finalIconIndex)) {
			psl->SetIconLocation(finalIconPath.c_str(), finalIconIndex);
		}

		// 4. 设置 AppUserModelID (确保任务栏合并正确)
		IPropertyStore* pps;
		hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
		if (SUCCEEDED(hr)) {
			PROPVARIANT var;
			// 假设 MY_APP_ID 是你定义的全局或常量 std::wstring
			hr = InitPropVariantFromString(cachedAppId.c_str(), &var);
			if (SUCCEEDED(hr)) {
				hr = pps->SetValue(PKEY_AppUserModel_ID, var);
				pps->Commit();
				PropVariantClear(&var);
			}
			pps->Release();
		}

		// 5. 保存快捷方式文件
		IPersistFile* ppf;
		hr = psl->QueryInterface(IID_PPV_ARGS(&ppf));
		if (SUCCEEDED(hr)) {
			hr = ppf->Save(lnkPath.c_str(), TRUE);
			ppf->Release();

			if (SUCCEEDED(hr)) {
				// 通知系统刷新图标缓存，使开始菜单立即显示新图标
				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
			}
		}
		psl->Release();
	}
	return hr;
}

// --- 修正后的通知函数 ---
static void ShowWin11NativeNotification(const std::wstring& title, const std::wstring& message)
{
	try {
		std::wstring xmlPayload =
			L"<toast><visual><binding template='ToastGeneric'>"
			L"<text>" + title + L"</text>"
			L"<text>" + message + L"</text>"
			L"</binding></visual></toast>";
		XmlDocument toastXml;
		toastXml.LoadXml(xmlPayload);
		// 显式指定类型避免 auto 推导错误
		ToastNotifier notifier = ToastNotificationManager::CreateToastNotifier(cachedAppId);
		ToastNotification notification(toastXml);
		notifier.Show(notification);
	}
	catch (...) {
		// 静默处理或输出调试信息
	}
}

// 任务栏图标相关
#define PROP_ORIGINAL_APPID L"MTU_ORIGINAL_APPID"

/**
 * 获取窗口原始图标并持久化到窗口属性中
 * @param hwnd 目标窗口句柄
 * @param isNewlyExtracted 返回值：本次是否新创建了图标句柄（需外部或销毁逻辑关注）
 */
HICON GetOriginalWindowIcon(HWND hwnd, bool& isNewlyExtracted) {
	isNewlyExtracted = false;

	// --- 1. 尝试从缓存读取 (LassoWindows 状态持久化) ---
	HICON hCachedIcon = (HICON)GetPropW(hwnd, L"MTU_ORIGINAL_ICON_KEEP");
	if (hCachedIcon != NULL) {
		// 验证句柄是否仍然有效
		ICONINFO ii = { 0 };
		if (GetIconInfo(hCachedIcon, &ii)) {
			if (ii.hbmColor) DeleteObject(ii.hbmColor);
			if (ii.hbmMask) DeleteObject(ii.hbmMask);
			return hCachedIcon;
		}
	}

	HICON hIcon = NULL;
	bool producedNewHandle = false;

	// --- 2. 像宿主请求图标 (WM_GETICON) ---
	// 使用带超时的消息发送，防止目标窗口挂起导致主线程死锁
	auto GetIconViaMessage = [](HWND targetHwnd, int iconType) -> HICON {
		DWORD_PTR result = 0;
		// 150ms 超时，避免阻塞
		if (SendMessageTimeoutW(targetHwnd, WM_GETICON, iconType, 0,
			SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 150, &result)) {
			return (HICON)result;
		}
		return NULL;
		};

	// 优先尝试大图标，再尝试小图标
	hIcon = GetIconViaMessage(hwnd, ICON_BIG);
	if (!hIcon) hIcon = GetIconViaMessage(hwnd, ICON_SMALL);

	// --- 3. 从窗口类中抓取 (Class Long) ---
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
	}

	// --- 4. 从可执行文件 (EXE) 路径提取 ---
	if (!hIcon) {
		DWORD pID = 0;
		GetWindowThreadProcessId(hwnd, &pID);
		if (pID != 0) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pID);
			if (hProcess) {
				wchar_t exePath[MAX_PATH] = { 0 };
				DWORD size = MAX_PATH;
				if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
					// 提取第一个图标资源
					if (ExtractIconExW(exePath, 0, &hIcon, NULL, 1) > 0) {
						producedNewHandle = true;
					}
				}
				CloseHandle(hProcess);
			}
		}
	}

	// --- 5. 最终收尾与持久化 ---
	if (hIcon) {
		SetPropW(hwnd, L"MTU_ORIGINAL_ICON_KEEP", (HANDLE)hIcon);

		// 修复 C4312 警告：通过 UINT_PTR 安全转换到 HANDLE
		UINT_PTR flag = producedNewHandle ? 1 : 0;
		SetPropW(hwnd, L"MTU_ICON_NEED_DESTROY", reinterpret_cast<HANDLE>(flag));
	}

	isNewlyExtracted = producedNewHandle;
	return hIcon;
}

// 任务栏图标变窗口+小图标
#pragma comment(lib, "dwmapi.lib")
// 辅助函数：更加健壮的 ITaskbarList3 初始化
ITaskbarList3* GetTaskbarList() {
	static ITaskbarList3* pTaskList = nullptr;
	if (!pTaskList) {
		// 使用 CoInitializeEx 代替 CoInitialize
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
			return nullptr;
		}
		hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskList));
		if (SUCCEEDED(hr) && pTaskList) {
			if (FAILED(pTaskList->HrInit())) {
				pTaskList->Release();
				pTaskList = nullptr;
			}
		}
	}
	return pTaskList;
}
//"剪影窗口"+右上角小图标提醒
void SetWindowSnapshotWithOverlay(HWND hwnd, HICON hCurrentIcon) {
	if (!IsWindow(hwnd)) return;
	// 确保我们操作的是顶级所有者窗口，任务栏图标挂载在它上面
	HWND hRoot = GetAncestor(hwnd, GA_ROOTOWNER);
	if (!hRoot) hRoot = hwnd;

	//// 1. 获取原图标 (Overlay) - 增加容错
	//HICON hOriginalIcon = (HICON)SendMessageW(hRoot, WM_GETICON, ICON_SMALL, 0);
	//if (!hOriginalIcon) hOriginalIcon = (HICON)GetClassLongPtrW(hRoot, GCLP_HICONSM);
	//// 如果还是没有，尝试获取 BIG 图标让系统自动缩放
	//if (!hOriginalIcon) hOriginalIcon = (HICON)SendMessageW(hRoot, WM_GETICON, ICON_BIG, 0);
	// 获取并还原原始图标

	WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
	if (!GetWindowPlacement(hRoot, &wp)) return;
	int winW = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
	int winH = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
	if (winW <= 0 || winH <= 0) return;
	int side = (std::max)(winW, winH);
	int offsetX = (side - winW) / 2;
	int offsetY = (side - winH) / 2;
	const int TargetIconSize = 128;
	int penWidth = (side > TargetIconSize) ? (int)ceil((float)side / (float)TargetIconSize) : 1;
	HDC hdcScreen = GetDC(NULL);
	if (!hdcScreen) return;
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	if (!hdcMem) {
		ReleaseDC(NULL, hdcScreen);
		return;
	}
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = side;
	bmi.bmiHeader.biHeight = -side;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	UINT32* pPixels = nullptr;
	HBITMAP hbmScreen = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0);
	if (hbmScreen && pPixels) {
		HGDIOBJ hOld = SelectObject(hdcMem, hbmScreen);
		memset(pPixels, 0, (size_t)side * side * sizeof(UINT32));
		UINT32 whitePixel = 0xFFFFFFFF;
		for (int y = 0; y < winH; ++y) {
			for (int x = 0; x < winW; ++x) {
				if (x < penWidth || x >= (winW - penWidth) || y < penWidth || y >= (winH - penWidth)) {
					int targetX = x + offsetX;
					int targetY = y + offsetY;
					if (targetX >= 0 && targetX < side && targetY >= 0 && targetY < side) {
						pPixels[targetY * side + targetX] = whitePixel;
					}
				}
			}
		}
		ICONINFO ii = { 0 };
		ii.fIcon = TRUE;
		ii.hbmColor = hbmScreen;
		ii.hbmMask = CreateBitmap(side, side, 1, 1, NULL);
		if (ii.hbmMask) {
			HICON hSnapshotIcon = CreateIconIndirect(&ii);
			if (hSnapshotIcon) {
				DWORD_PTR dwResult;
				// 设置主图标
				if (SendMessageTimeoutW(hRoot, WM_SETICON, ICON_BIG, (LPARAM)hSnapshotIcon,
					SMTO_ABORTIFHUNG, 1000, &dwResult)) {
					HANDLE hOldIcon = GetPropW(hRoot, L"MTU_SNAPSHOT_ICON");
					if (hOldIcon) DestroyIcon((HICON)hOldIcon);
					SetPropW(hRoot, L"MTU_SNAPSHOT_ICON", (HANDLE)hSnapshotIcon);
					// --- 关键修正：确保在主图标设置后设置 Overlay ---
					ITaskbarList3* pTaskList = GetTaskbarList();
					if (pTaskList) {
						// 某些情况下需要先尝试 AddTab 确保窗口在任务栏受控
						pTaskList->AddTab(hRoot);
						HRESULT hrOverlay = pTaskList->SetOverlayIcon(hRoot, hCurrentIcon, L"Active");
						// 如果失败，可能是因为图标句柄问题，尝试传 NULL 清除再重设
						if (FAILED(hrOverlay)) {
							pTaskList->SetOverlayIcon(hRoot, NULL, NULL);
						}
					}
				}
				else {
					DestroyIcon(hSnapshotIcon);
				}
			}
			DeleteObject(ii.hbmMask);
		}
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmScreen);
	}
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

//新设appid，使得任务栏图标独立
HRESULT SetWindowAppIdWithBackup(HWND hwnd, PCWSTR newAppId) {
	if (!IsWindow(hwnd)) return E_INVALIDARG;

	// 获取并还原原始图标
	bool isNewlyExtracted = false;
	HICON hBackupIcon = GetOriginalWindowIcon(hwnd, isNewlyExtracted);

	IPropertyStore* pps;
	HRESULT hr = SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&pps));
	if (SUCCEEDED(hr)) {
		// --- 备份逻辑 ---
		if (GetPropW(hwnd, PROP_ORIGINAL_APPID) == NULL) {
			PROPVARIANT varOld;
			PropVariantInit(&varOld);
			if (SUCCEEDED(pps->GetValue(PKEY_AppUserModel_ID, &varOld))) {
				if (varOld.vt == VT_LPWSTR && varOld.pwszVal != NULL && varOld.pwszVal[0] != L'\0') {
					ATOM backupAtom = GlobalAddAtomW(varOld.pwszVal);
					SetPropW(hwnd, PROP_ORIGINAL_APPID, (HANDLE)(UINT_PTR)backupAtom);
				}
				else {
					SetPropW(hwnd, PROP_ORIGINAL_APPID, (HANDLE)(UINT_PTR)0xFFFF);
				}
				PropVariantClear(&varOld);
			}
		}
		// --- 设置新的唯一 AppID ---
		PROPVARIANT varNew;
		if (SUCCEEDED(InitPropVariantFromString(newAppId, &varNew))) {
			// 在 Commit 之前，先利用我们抓到的 hBackupIcon 做一次“闪电注入”
			// 这样当 ID 变更的一瞬间，任务栏看到的不是空的，而是我们注入的图标
			if (hBackupIcon) {
				SendMessageTimeoutW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hBackupIcon, SMTO_ABORTIFHUNG, 100, NULL);
			}
			hr = pps->SetValue(PKEY_AppUserModel_ID, varNew);
			if (SUCCEEDED(hr)) hr = pps->Commit();
			PropVariantClear(&varNew);
		}
		pps->Release();
	}
	if (SUCCEEDED(hr)) {
		// --- 强制刷新 ---
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
		// 此时 SetWindowSnapshotWithOverlay 会生成带白边的缩影，并覆盖掉上面的 hBackupIcon
		SetWindowSnapshotWithOverlay(hwnd, hBackupIcon);
	}
	// 清理临时抓取的图标（如果是 ExtractIcon 出来的）
	// 注意：如果 hBackupIcon 是 SendMessage 拿到的引用，通常不需要 Destroy，
	// 但为了安全，建议判断是否与窗口当前图标一致。
	return hr;
}

///**
// * 彻底注销显式 ID 并使窗口“归队”
// */
void RestoreWindowAppId(HWND hwnd) {
	HANDLE hData = GetPropW(hwnd, PROP_ORIGINAL_APPID);
	if (!hData) return;
	// --- 1. 显式清除 Overlay (防止归队后把 Overlay 带回原组) ---
	ITaskbarList3* pTaskbarList = GetTaskbarList(); // 使用之前定义的单例获取函数
	if (pTaskbarList) {
		//pTaskbarList->SetOverlayIcon(hwnd, static_cast<HICON>(NULL), NULL);
		pTaskbarList->DeleteTab(hwnd); // A: 彻底离队
	}
	// --- 2. 清理快照图标资源 ---
	HANDLE hSnapshotIcon = RemovePropW(hwnd, L"MTU_SNAPSHOT_ICON");
	if (hSnapshotIcon) {
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, NULL);
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, NULL);
		DestroyIcon((HICON)hSnapshotIcon);
	}
	// --- 3. 彻底重置 AppID 属性 ---
	IPropertyStore* pps = NULL;
	if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&pps)))) {
		UINT_PTR val = (UINT_PTR)hData;
		if (val == 0xFFFF) {
			PROPVARIANT varEmpty;
			PropVariantInit(&varEmpty);
			pps->SetValue(PKEY_AppUserModel_ID, varEmpty);
		}
		else {
			wchar_t oldId[MAX_PATH];
			if (GlobalGetAtomNameW((ATOM)val, oldId, MAX_PATH)) {
				PROPVARIANT varOld;
				if (SUCCEEDED(InitPropVariantFromString(oldId, &varOld))) {
					pps->SetValue(PKEY_AppUserModel_ID, varOld);
					PropVariantClear(&varOld);
				}
				GlobalDeleteAtom((ATOM)val);
			}
		}
		pps->Commit();
		pps->Release();
	}
	RemovePropW(hwnd, PROP_ORIGINAL_APPID);
	// --- 4. 重新入队并强制刷新 ---
	if (pTaskbarList) {
		pTaskbarList->AddTab(hwnd);
	}
	// 触发重绘
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}


//Todolist：窗口自动随桌面切换，避免 win+tab 时窗口被拉回原桌面
// win+tab 将有关联窗口拉取到当前桌面
// 获取虚拟桌面名称
#include <rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
std::wstring GetCurrentVirtualDesktopName(std::wstring forWhichPurpose) {
	HKEY hKey;
	BYTE guid[16];
	DWORD size = sizeof(guid);
	std::wstring result = L"";
	bool bNameFound = false;
	const wchar_t* rootPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";
	if (RegOpenKeyExW(HKEY_CURRENT_USER, rootPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueExW(hKey, L"CurrentVirtualDesktop", NULL, NULL, guid, &size) == ERROR_SUCCESS) {
			RPC_WSTR guidStr;
			if (UuidToStringW((UUID*)guid, &guidStr) == RPC_S_OK) {
				std::wstring subKeyPath = std::wstring(rootPath) + L"\\Desktops\\{" + (wchar_t*)guidStr + L"}";
				RpcStringFreeW(&guidStr);
				HKEY hSubKey;
				if (RegOpenKeyExW(HKEY_CURRENT_USER, subKeyPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
					wchar_t nameBuffer[512] = { 0 };
					DWORD nameSize = sizeof(nameBuffer);
					if (RegQueryValueExW(hSubKey, L"Name", NULL, NULL, (LPBYTE)nameBuffer, &nameSize) == ERROR_SUCCESS) {
						result = nameBuffer;
						bNameFound = true; // 只有真正读到用户定义的名称，才算成功
					}
					RegCloseKey(hSubKey);
				}
			}
		}
		RegCloseKey(hKey);
	}
	// 6. 严密拦截：如果没有成功获取到桌面名称，触发你的原生通知窗口
	if (!bNameFound) {
		// 此时由于没有获取到真实桌面名，传入通知的 currentVd 暂时用提示语代替
		ShowWin11NativeNotification(L"未命名或未识别的虚拟桌面", forWhichPurpose.c_str());
	}

	return result;
}

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// 判断窗口是否在当前活跃的虚拟桌面
bool IsWindowOnCurrentVirtualDesktop(HWND hWnd) {
	IVirtualDesktopManager* pDesktopManager = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDesktopManager));

	BOOL isOnCurrent = FALSE;
	if (SUCCEEDED(hr)) {
		// 这个方法会返回该 HWND 是否在当前用户可见的桌面上
		hr = pDesktopManager->IsWindowOnCurrentVirtualDesktop(hWnd, &isOnCurrent);
		pDesktopManager->Release();
	}
	return (SUCCEEDED(hr) && isOnCurrent);
}

// 事件监控相关
// --- 全局配置与状态 ---
#define TIMER_ID_COLLECT 1001
bool g_InSnapWindows = false;
// 全局句柄
HHOOK g_hKeyHook = NULL;
HHOOK g_hMouseHook = NULL;
HWINEVENTHOOK g_hSysHook = NULL;
HWINEVENTHOOK g_hObjHook = NULL;

// 建议：句柄缓存 - 避免频繁 EnumWindows 带来的性能开销
std::vector<HWND> g_HiddenWindowsCache;

// Snapshot state once; a temporary Alt+Tab preview must not overwrite it.
struct SealedWindowState {
    LONG_PTR exStyle = 0;
    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    HWND previous = nullptr;
    BYTE alpha = 255;
    COLORREF colorKey = 0;
    DWORD layerFlags = 0;
    bool hasLayerAttributes = false;
    bool wasVisible = false;
};
static std::unordered_map<HWND, SealedWindowState> g_sealedWindowStates;

static bool SnapshotWindowState(HWND h) {
    if (!IsWindow(h)) return false;
    if (g_sealedWindowStates.contains(h)) return true;
    SealedWindowState state;
    state.exStyle = GetWindowLongPtrW(h, GWL_EXSTYLE);
    state.previous = GetWindow(h, GW_HWNDPREV);
    state.wasVisible = IsWindowVisible(h) != FALSE;
    if (!GetWindowPlacement(h, &state.placement)) return false;
    // Do not alter a pre-existing layered window if its original alpha is
    // unavailable: no lossless restoration would be possible.
    if (state.exStyle & WS_EX_LAYERED) {
        if (!GetLayeredWindowAttributes(h, &state.colorKey, &state.alpha, &state.layerFlags))
            return false;
        state.hasLayerAttributes = true;
    }
    g_sealedWindowStates.emplace(h, state);
    return true;
}

// preview=true retains the original snapshot for a later re-seal.
void ShowWindowAndShowTaskbarIcon(HWND h, bool preview = false) {
    auto it = g_sealedWindowStates.find(h);
    if (it == g_sealedWindowStates.end()) return;
    if (!IsWindow(h)) { g_sealedWindowStates.erase(it); return; }
    const SealedWindowState original = it->second;

    SetWindowLongPtrW(h, GWL_EXSTYLE, original.exStyle);
    if (original.hasLayerAttributes) {
        SetLayeredWindowAttributes(h, original.colorKey, original.alpha, original.layerFlags);
    }
    HWND after = (original.previous && IsWindow(original.previous))
        ? original.previous : HWND_TOP;
    SetWindowPos(h, after, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (!original.wasVisible) ShowWindow(h, SW_HIDE);
    else if (original.placement.showCmd == SW_SHOWMINIMIZED ||
             original.placement.showCmd == SW_MINIMIZE ||
             original.placement.showCmd == SW_SHOWMINNOACTIVE)
        ShowWindow(h, SW_SHOWMINNOACTIVE);
    else ShowWindow(h, SW_SHOWNOACTIVATE);

    ITaskbarList* taskbar = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList, reinterpret_cast<void**>(&taskbar))) && taskbar) {
        taskbar->HrInit();
        if (original.wasVisible && !(original.exStyle & WS_EX_TOOLWINDOW))
            taskbar->AddTab(h);
        taskbar->Release();
    }
    InvalidateRect(h, nullptr, TRUE);
    if (!preview) g_sealedWindowStates.erase(it);
}

void HideWindowAndHideTaskbarIcon(HWND h) {
    if (!SnapshotWindowState(h)) return;
    if (IsIconic(h)) ShowWindow(h, SW_RESTORE);
    LONG_PTR exStyle = GetWindowLongPtrW(h, GWL_EXSTYLE);
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT;
    SetWindowLongPtrW(h, GWL_EXSTYLE, exStyle);
    SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
    SetWindowPos(h, HWND_BOTTOM, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ITaskbarList* taskbar = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList, reinterpret_cast<void**>(&taskbar))) && taskbar) {
        taskbar->HrInit();
        taskbar->DeleteTab(h);
        taskbar->Release();
    }
}

static void RestoreAllSealedWindows() {
    while (!g_sealedWindowStates.empty()) {
        HWND h = g_sealedWindowStates.begin()->first;
        ShowWindowAndShowTaskbarIcon(h);
    }
}

// --- 逻辑 1：全局压制逻辑 ---
void SuppressHiddenWindows(HWND hwnd, DWORD event) {
	// 只有在非任务视图模式下才执行压制
	if (event == EVENT_OBJECT_SHOW && !g_InSnapWindows) {
		// 关键：只要 GetPropW 返回非空，说明窗口已被标记，直接压制
		if (GetPropW(hwnd, L"MTU_PROC_TAG")) {
			HideWindowAndHideTaskbarIcon(hwnd);
		}
	}
}

void RefreshWindowCache() {
	g_HiddenWindowsCache.clear();

	// 1. 初始化 COM 接口（建议在主线程初始化一次，这里为了演示逻辑完整性）
	IVirtualDesktopManager* pVDM = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pVDM));

	if (FAILED(hr)) {
		// 如果接口获取失败（极少数旧系统情况），回退到基础逻辑
		return;
	}

	// 2. 遍历窗口
	EnumWindows([](HWND h, LPARAM lp) -> BOOL {
		IVirtualDesktopManager* pManager = reinterpret_cast<IVirtualDesktopManager*>(lp);

		// 只有我们标记过的“封印”窗口才进缓存
		if (GetPropW(h, L"MTU_PROC_TAG")) {
			BOOL isCurrent = FALSE;

			// --- 核心判定：由系统告诉你这个窗口是否在当前活跃桌面 ---
			if (SUCCEEDED(pManager->IsWindowOnCurrentVirtualDesktop(h, &isCurrent))) {
				if (isCurrent) {
					// 在当前桌面，加入待管理缓存
					g_HiddenWindowsCache.push_back(h);
				}
				else {
					// 不在当前桌面，强制隐藏（防止跨桌面残留）
					if (IsWindowVisible(h)) {
						HideWindowAndHideTaskbarIcon(h);
					}
				}
			}
		}
		return TRUE;
		}, (LPARAM)pVDM);

	// 3. 释放接口
	pVDM->Release();
}



#include <commctrl.h>
// 显式链接库文件
#pragma comment(lib, "comctl32.lib")
// --- 定义自定义消息 ---
// 使用 WM_USER + 某个数字，确保不与系统消息冲突
#define WM_LASSO_SYNC_PROCESS (WM_USER + 101) 
// --- 定义全局共享缓冲区 ---
// 用于在 Hook 线程和 Coordinator 线程之间传递字符串
wchar_t g_SharedProcName[MAX_PATH] = { 0 };
// --- 声明协调器句柄 ---
HWND g_hLassoCoordinator = NULL;

// 辅助结构体，用于在子窗口中搜寻 PID
struct UWP_SPY_INFO {
	DWORD hostPid;   // ApplicationFrameHost 的 PID
	DWORD realPid;   // 真正的业务进程 PID
};

// 示例：获取窗口对应的真实进程名（已穿透 ApplicationFrameHost）
std::wstring GetRealProcessName(HWND hwnd) {
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);
	if (processId == 0) return L"Unknown";

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (!hProcess) return L"Unknown";

	wchar_t imagePath[MAX_PATH];
	DWORD size = MAX_PATH;
	std::wstring result = L"Unknown";

	if (QueryFullProcessImageNameW(hProcess, 0, imagePath, &size)) {
		std::wstring fullPath = imagePath;
		size_t lastSlash = fullPath.find_last_of(L"\\");
		std::wstring fileName = (lastSlash == std::wstring::npos) ? fullPath : fullPath.substr(lastSlash + 1);

		// --- UWP 深度识别逻辑 ---
		if (_wcsicmp(fileName.c_str(), L"ApplicationFrameHost.exe") == 0) {
			UWP_SPY_INFO spy = { processId, 0 };

			// 枚举子窗口，寻找属于不同进程的窗口句柄
			EnumChildWindows(hwnd, [](HWND hChild, LPARAM lp) -> BOOL {
				UWP_SPY_INFO* info = reinterpret_cast<UWP_SPY_INFO*>(lp);
				DWORD cPid = 0;
				GetWindowThreadProcessId(hChild, &cPid);

				if (cPid != 0 && cPid != info->hostPid) {
					info->realPid = cPid;
					return FALSE; // 找到真身，停止枚举
				}
				return TRUE;
				}, reinterpret_cast<LPARAM>(&spy));

			// 如果找到了业务 PID，获取它的真实进程名
			if (spy.realPid != 0) {
				HANDLE hRealProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, spy.realPid);
				if (hRealProc) {
					wchar_t realPath[MAX_PATH];
					DWORD rSize = MAX_PATH;
					if (QueryFullProcessImageNameW(hRealProc, 0, realPath, &rSize)) {
						std::wstring realFullPath = realPath;
						size_t rLastSlash = realFullPath.find_last_of(L"\\");
						result = (rLastSlash == std::wstring::npos) ? realFullPath : realFullPath.substr(rLastSlash + 1);
					}
					CloseHandle(hRealProc);
				}
			}
			else {
				result = fileName; // 没找到子进程，回退到宿主名
			}
		}
		else {
			// 普通 Win32 程序直接返回文件名
			result = fileName;
		}
	}

	CloseHandle(hProcess);
	return result;
}

void CALLBACK OnCollectTimer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	KillTimer(NULL, idEvent);

	HWND hForeground = GetForegroundWindow();
	if (!hForeground) return;

	// --- 1. 核心防御：如果还在任务切换界面，绝对不收网 ---
	wchar_t className[256] = { 0 };
	wchar_t windowTitle[256] = { 0 };
	GetClassNameW(hForeground, className, 256);
	GetWindowTextW(hForeground, windowTitle, 256);

	// 排除任务视图、Alt+Tab 界面、以及开始菜单等干扰
	if (wcscmp(className, L"XamlExplorerHostIslandWindow") == 0 &&
		wcscmp(windowTitle, L"任务切换") == 0)
	{
		// 还没退出任务视图，用户可能还在选，续命定时器
		SetTimer(NULL, idEvent, 100, OnCollectTimer);
		return;
	}

	g_InSnapWindows = false;

	// --- 2. 判定真正的胜出者 ---
	HWND hWinner = GetAncestor(hForeground, GA_ROOTOWNER);

	// 优先读取前台窗口的进程 Atom 标记；若没有，则实时提取真实进程名匹配
	HANDLE hTargetProp = GetPropW(hWinner, L"MTU_PROC_TAG");
	std::wstring targetProcName;

	if (hTargetProp) {
		wchar_t buf[MAX_PATH];
		GlobalGetAtomNameW(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hTargetProp)), buf, MAX_PATH);
		targetProcName = buf;
	}
	else {
		targetProcName = GetRealProcessName(hWinner);
	}

	// 如果前台窗口既无 Tag 也非被隐匿的目标进程，说明用户选中了无关窗口：全数压制回隐藏缓存
	if (targetProcName.empty()) {
		for (HWND h : g_HiddenWindowsCache) {
			if (IsWindow(h) && IsWindowVisible(h)) HideWindowAndHideTaskbarIcon(h);
		}
		return;
	}

	std::wstring currentVd = GetCurrentVirtualDesktopName(L"等待器");

	// --- 3. 精准收网：遍历缓存，同进程且同桌面的所有窗口统一解封，其余继续压制 ---
	g_HiddenWindowsCache.erase(
		std::remove_if(g_HiddenWindowsCache.begin(), g_HiddenWindowsCache.end(),
			[&](HWND h) {
				if (!IsWindow(h)) return true;

				HANDLE hProc = GetPropW(h, L"MTU_PROC_TAG");
				HANDLE hVd = GetPropW(h, L"MTU_VD_TAG");

				wchar_t pName[MAX_PATH] = { 0 };
				wchar_t vName[MAX_PATH] = { 0 };

				if (hProc) GlobalGetAtomNameW(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hProc)), pName, MAX_PATH);
				if (hVd) GlobalGetAtomNameW(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hVd)), vName, MAX_PATH);

				// 判断是否为同一进程
				bool isSameProc = (!hProc && GetRealProcessName(h) == targetProcName) ||
					(hProc && wcscmp(targetProcName.c_str(), pName) == 0);

				// 判断是否处于同一虚拟桌面
				bool isSameVd = (hVd && wcscmp(currentVd.c_str(), vName) == 0) ||
					IsWindowOnCurrentVirtualDesktop(h);

				// 【同进程 + 同桌面】统一完全还原解封
				if (isSameProc && isSameVd) {

					// A. 还原标题 (去掉前缀 🕸)
					wchar_t currentTitle[512];
					if (GetWindowTextW(h, currentTitle, 512) > 0) {
						std::wstring t = currentTitle;
						std::wstring prefix = L"🕸";

						if (t.find(prefix) == 0) {
							t.erase(0, prefix.length());
							SetWindowTextW(h, t.c_str());
						}
					}

					// B. 清理附加 Atom
					if (hProc) {
						RemovePropW(h, L"MTU_PROC_TAG");
						GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hProc)));
					}
					if (hVd) {
						RemovePropW(h, L"MTU_VD_TAG");
						GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hVd)));
					}

					// C. 完全恢复任务栏图标与可见性
					ShowWindowAndShowTaskbarIcon(h);

					return true; // 从隐藏缓存列表中移除
				}

				// 未选中的同桌面窗口继续维持压制
				if (IsWindowOnCurrentVirtualDesktop(h)) {
					HideWindowAndHideTaskbarIcon(h);
				}
				return false;
			}),
		g_HiddenWindowsCache.end()
	);
}

//realname一致的同类窗口处理
struct CloseFilter {
    std::wstring currentAppProcName;
};

// 隐藏指定 App 进程名对应的窗口
BOOL CALLBACK EnumHideWindowsProc(HWND hWnd, LPARAM lParam) {
	if (!lParam) return FALSE;

	const std::wstring& targetProcName = *(reinterpret_cast<const std::wstring*>(lParam));
	if (targetProcName.empty()) return TRUE;

	// 基础过滤：仅处理可见顶级窗口
	if (!IsWindow(hWnd) || !IsWindowVisible(hWnd) || GetParent(hWnd) != NULL) {
		return TRUE;
	}

	// 虚拟桌面过滤
	if (!IsWindowOnCurrentVirtualDesktop(hWnd)) {
		return TRUE;
	}

	std::wstring currentWndProcName = GetRealProcessName(hWnd);

	if (_wcsicmp(targetProcName.c_str(), currentWndProcName.c_str()) == 0) {
		wchar_t className[256] = { 0 };
		if (GetClassNameW(hWnd, className, 256)) {
			// Corner Case: explorer.exe 过滤
			if (_wcsicmp(currentWndProcName.c_str(), L"explorer.exe") == 0) {
				if (_wcsicmp(className, L"CabinetWClass") != 0) {
					return TRUE;
				}
			}
		}

		// --- A. 添加 “🕸” 蛛网标 ---
		wchar_t originalTitle[512] = { 0 };
		if (GetWindowTextW(hWnd, originalTitle, 512) > 0) {
			std::wstring titleStr = originalTitle;
			std::wstring prefix = L"🕸";

			if (titleStr.find(prefix) != 0) {
				std::wstring newTitle = prefix + titleStr;
				SetWindowTextW(hWnd, newTitle.c_str());
				SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
			}
		}

		// --- B. 挂载进程名 Atom (MTU_PROC_TAG) ---
		if (!GetPropW(hWnd, L"MTU_PROC_TAG")) {
			ATOM procAtom = GlobalAddAtomW(targetProcName.c_str());
			SetPropW(hWnd, L"MTU_PROC_TAG", reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(procAtom)));
		}

		// --- C. 挂载虚拟桌面 Atom (MTU_VD_TAG) ---
		if (!GetPropW(hWnd, L"MTU_VD_TAG")) {
			std::wstring vDesktopName = GetCurrentVirtualDesktopName(L"隐藏当前程序窗口");
			if (!vDesktopName.empty()) {
				ATOM vdAtom = GlobalAddAtomW(vDesktopName.c_str());
				SetPropW(hWnd, L"MTU_VD_TAG", reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(vdAtom)));
			}
		}

		// --- D. 执行隐匿 ---
		HideWindowAndHideTaskbarIcon(hWnd);
	}

	return TRUE;
}

// 反向隐藏：隐藏所有【不属于】当前 App 进程名的窗口
BOOL CALLBACK EnumHideOthersProc(HWND hWnd, LPARAM lParam) {
	CloseFilter* filter = reinterpret_cast<CloseFilter*>(lParam);
	if (!filter) return FALSE;

	const std::wstring& currentAppProcName = filter->currentAppProcName;

	// 1. 基础过滤：跳过不可见窗口、子窗口、以及我们自己的管理窗口
	if (!IsWindowVisible(hWnd) || GetParent(hWnd) != NULL) {
		return TRUE;
	}

	// --- 【关键新增】：只处理当前虚拟桌面上的窗口 ---
	// 如果窗口在其他桌面，直接跳过，不进行匹配和隐藏
	if (!IsWindowOnCurrentVirtualDesktop(hWnd)) {
		return TRUE;
	}

	// 2. 获取遍历到的窗口所属进程名
	std::wstring otherWndProcName = GetRealProcessName(hWnd);

	// 3. 核心逻辑：如果这个窗口【不属于】当前正在使用的 App
	if (_wcsicmp(otherWndProcName.c_str(), currentAppProcName.c_str()) != 0) {

		// --- 关键修正：仿照 EnumHideWindowsProc 的安全过滤 ---

		// =========================================================================
		// 【完美改进版】：反向隐藏遍历回调中的关闭/过滤逻辑
		// =========================================================================
		wchar_t className[256] = { 0 };
		if (GetClassNameW(hWnd, className, 256)) {

			// A. 针对 explorer.exe 的精准过滤
			if (_wcsicmp(otherWndProcName.c_str(), L"explorer.exe") == 0) {
				wchar_t className[256];
				if (GetClassNameW(hWnd, className, 256)) {
					// 在反向隐藏时，explorer 进程中我们只敢隐藏“文件夹窗口”
					// 必须跳过 Shell_TrayWnd, Progman, WorkerW 等所有其他组件
					if (_wcsicmp(className, L"CabinetWClass") != 0) {
						return TRUE;
					}
				}
			}

			// ---------------------------------------------------------------------
			// 分支 B：处理 UWP 托管进程 (ApplicationFrameHost.exe / ApplicationFrameWindow)
			// 核心修正：将其移出 explorer 作用域，在外层进行平级判定
			// ---------------------------------------------------------------------
			// UWP windows use the same reversible seal; never send WM_CLOSE.
		}

		// --- 1. 获取并修改标题文字 ---		}

		// --- 1. 获取并修改标题文字 ---
		wchar_t originalTitle[512];
		if (GetWindowTextW(hWnd, originalTitle, 512) > 0) {
			std::wstring titleStr = originalTitle;
			std::wstring prefix = L"🕸";

			// 检查是否已经贴过标，防止重复添加
			if (titleStr.find(prefix) != 0) {
				std::wstring newTitle = prefix + titleStr;
				SetWindowTextW(hWnd, newTitle.c_str());
			}
		}
		else {
			// 如果窗口本来没有标题，直接设置
			SetWindowTextW(hWnd, L"🕸");
		}

		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

		// --- 动态贴标逻辑 ---
		// 标记该窗口所属的进程名，以便后续通过 Alt+Tab 恢复该进程的所有窗口
		if (!GetPropW(hWnd, L"MTU_PROC_TAG")) {
			ATOM procAtom = GlobalAddAtomW(otherWndProcName.c_str());
			SetPropW(hWnd, L"MTU_PROC_TAG", (HANDLE)(UINT_PTR)procAtom);
		}

		// 2. 设置虚拟桌面名称标签 (新增逻辑)
		std::wstring forWhichPurpose = L"隐藏其它窗口";
		std::wstring vDesktopName = GetCurrentVirtualDesktopName(forWhichPurpose); // 这里替换为你获取名称的逻辑
		if (!GetPropW(hWnd, L"MTU_VD_TAG")) {
			ATOM vdAtom = GlobalAddAtomW(vDesktopName.c_str());
			SetPropW(hWnd, L"MTU_VD_TAG", (HANDLE)(UINT_PTR)vdAtom);
		}

		// --- 执行隐藏 ---
		HideWindowAndHideTaskbarIcon(hWnd);
	}

	return TRUE;
}



// 隐藏同类型窗口后，alt+tab临时查看下。
// --- 全局变量与状态控制 ---
bool g_AltLogicActive = false;  // 追踪 Alt 物理按下状态
std::vector<HWND> g_snapUnhiddenWindows; // 存储 Alt+Tab 临时反隐藏的窗口句柄列表
// ============================================================================
// 1. 现代按键模拟辅助函数 (基于 SendInput)
// ============================================================================

// 模拟 Tab 键按下与抬起
void TriggerTabKeyModern() {
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_TAB;
	inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKeyW(VK_TAB, MAPVK_VK_TO_VSC));

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_TAB;
	inputs[1].ki.wScan = static_cast<WORD>(MapVirtualKeyW(VK_TAB, MAPVK_VK_TO_VSC));
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));
}

// 模拟无名键以消除 Win 键抬起时触发开始菜单的干扰
void SuppressWinKeyMenuModern() {
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_NONAME;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_NONAME;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));
}

// ============================================================================
// Alt+Tab 反隐藏及收网逻辑修正版（支持同进程多窗口统一解封与还原）
// ============================================================================

bool TryUnHideWindowsForAltTab() {
	RefreshWindowCache();
	if (g_HiddenWindowsCache.empty()) return false;

	bool didShowAny = false;
	std::wstring currentVd = GetCurrentVirtualDesktopName(L"键盘钩子");

	// 每次重新快照前清空快照暂存列表
	g_snapUnhiddenWindows.clear();

	for (HWND h : g_HiddenWindowsCache) {
		if (!IsWindow(h)) continue;

		// 【关键防御】：检查虚拟桌面匹配
		HANDLE hVdProp = GetPropW(h, L"MTU_VD_TAG");
		if (!hVdProp) continue;

		ATOM vdAtom = static_cast<ATOM>(reinterpret_cast<UINT_PTR>(hVdProp));
		wchar_t savedVdName[MAX_PATH];

		if (GlobalGetAtomNameW(vdAtom, savedVdName, MAX_PATH)) {
			if (wcscmp(savedVdName, currentVd.c_str()) == 0) {

				// A. 直接解封：剥离 WS_EX_TOOLWINDOW，恢复 WS_EX_APPWINDOW 与窗口渲染
				ShowWindowAndShowTaskbarIcon(h);

				// B. 记录到快照暂存列表，用于后续精确收网
				g_snapUnhiddenWindows.push_back(h);
				didShowAny = true;
			}
			else {
				// 不在当前虚拟桌面的窗口，继续维持最小化与任务栏隐匿
				HideWindowAndHideTaskbarIcon(h);
			}
		}
	}

	if (didShowAny) {
		g_InSnapWindows = true;
		KillTimer(NULL, TIMER_ID_COLLECT);

		// 阻塞等待 DWM 合成最新视口纹理，确保 Alt+Tab 捕捉到不黑屏的缩略图
		DwmFlush();
	}

	return didShowAny;
}

/**
 * @brief 同步收网还原核心函数：将 Alt+Tab 快照阶段临时还原的窗口重新切回最小化隐匿状态
 */
void ExecuteCollectRestore() {
	// 1. 如果当前并不处于快照状态，直接退出
	if (!g_InSnapWindows) {
		return;
	}

	// 2. 精准重新压制快照列表中的主窗口
	for (HWND hWnd : g_snapUnhiddenWindows) {
		if (IsWindow(hWnd)) {
			HideWindowAndHideTaskbarIcon(hWnd);
		}
	}
	g_snapUnhiddenWindows.clear();

	// 3. 重置状态机
	g_InSnapWindows = false;
	g_AltLogicActive = false;

	// 4. 销毁定时器
	KillTimer(NULL, TIMER_ID_COLLECT);
}

// ============================================================================
// 3. 核心钩子回调函数
// ============================================================================

LRESULT CALLBACK LowLevelKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KBDLLHOOKSTRUCT* pKeyStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

	// 1. 物理 Alt 键状态复位
	if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) {
		g_AltLogicActive = false;
	}

	// --- Cancel 机制：拦截 Esc 键 ---
	if (g_InSnapWindows && pKeyStruct->vkCode == VK_ESCAPE) {
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			ExecuteCollectRestore(); // 立即收网，恢复最小化隐匿状态
		}
	}

	// --- 分支 A：处理 Alt + Tab (反隐藏逻辑) ---
	if (pKeyStruct->vkCode == VK_TAB && (pKeyStruct->flags & LLKHF_ALTDOWN)) {
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {

			if (pKeyStruct->flags & LLKHF_INJECTED) {
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			}

			// 执行过滤反隐藏，恢复图标与视口以供 Alt+Tab 预览
			if (TryUnHideWindowsForAltTab()) {
				TriggerTabKeyModern();
				return 1;
			}
		}
	}

	// --- 分支 B：处理 Alt 键生命周期 ---
	if (pKeyStruct->vkCode == VK_MENU || pKeyStruct->vkCode == VK_LMENU || pKeyStruct->vkCode == VK_RMENU) {
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			g_AltLogicActive = true;
		}
		else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			g_AltLogicActive = false;
			// Alt 抬起，延迟 150ms 触发收网
			SetTimer(NULL, TIMER_ID_COLLECT, 150, reinterpret_cast<TIMERPROC>(OnCollectTimer));
		}
	}

	// 提取 Win 与 Shift 按键状态
	bool winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
	bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000);

	// --- 分支 D：Shift + Win + C (独占模式) ---
	if (winPressed && shiftPressed && pKeyStruct->vkCode == 'C' && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		HWND hCurr = GetForegroundWindow();
		if (hCurr) {
			HWND hRoot = GetAncestor(hCurr, GA_ROOT);
			wchar_t className[256];
			if (GetClassNameW(hRoot, className, 256)) {
				if (wcscmp(className, L"Progman") == 0 ||
					wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
					wcscmp(className, L"Shell_TrayWnd") == 0) {
					SuppressWinKeyMenuModern();
					return 1;
				}
			}

			std::wstring currentAppProcName = GetRealProcessName(hRoot);
			EnumWindows(EnumHideOthersProc, reinterpret_cast<LPARAM>(&currentAppProcName));
		}
		SuppressWinKeyMenuModern();
		return 1;
	}

	// --- 分支 C：Win + C (隐藏当前应用) ---
	if (winPressed && pKeyStruct->vkCode == 'C' && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		HWND hCurr = GetForegroundWindow();
		if (hCurr) {
			HWND hRoot = GetAncestor(hCurr, GA_ROOT);

			wchar_t className[256];
			if (GetClassNameW(hRoot, className, 256)) {
				if (wcscmp(className, L"ApplicationFrameWindow") == 0) {
					CloseFilter filter;
					wcscpy_s(filter.targetClassName, className);
					filter.targetRealProcName = GetRealProcessName(hRoot);

					EnumWindows(EnumCloseApplicationFrameWindowProc, reinterpret_cast<LPARAM>(&filter));
					SuppressWinKeyMenuModern();
					return 1;
				}

				if (wcscmp(className, L"Progman") == 0 ||
					wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
					wcscmp(className, L"Shell_TrayWnd") == 0 ||
					wcscmp(className, L"XamlExplorerHostIslandWindow") == 0) {
					SuppressWinKeyMenuModern();
					return 1;
				}
			}

			std::wstring procName = GetRealProcessName(hRoot);

			if (IsWindow(g_hLassoCoordinator)) {
				wcsncpy_s(g_SharedProcName, procName.c_str(), _TRUNCATE);
				SendMessageW(g_hLassoCoordinator, WM_LASSO_SYNC_PROCESS, reinterpret_cast<WPARAM>(hRoot), 0);
			}

			EnumWindows(EnumHideWindowsProc, reinterpret_cast<LPARAM>(&procName));
		}

		SuppressWinKeyMenuModern();
		return 1;
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}



// 主桌面右下角点击后显示桌面图标，这里做一个鼠标左键点击劫持
// 表现为只将主桌面窗口隐藏，附加桌面不影响。
// 全局状态控制
const int TARGET_ZONE_WIDTH = 30; // 右下角触发区域宽度

// 检查窗口是否是用户可见的、合法的普通主窗口
BOOL IsValidAppWindow(HWND hWnd) {
	if (GetParent(hWnd) != NULL) return FALSE;

	wchar_t className[256];
	if (GetClassNameW(hWnd, className, 256)) {
		if (wcscmp(className, L"Progman") == 0 ||
			wcscmp(className, L"WorkerW") == 0 ||
			wcscmp(className, L"Shell_TrayWnd") == 0 ||
			wcscmp(className, L"Shell_SecondaryTrayWnd") == 0) {
			return FALSE;
		}
	}

	LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
	if (exStyle & WS_EX_TOOLWINDOW) return FALSE;

	return TRUE;
}

// 第一次修改
#include <ShellScalingApi.h> // 👈 包含此头文件

#pragma comment(lib, "Shcore.lib") // 👈 链接 Shcore 库

// 在全局变量或单例中维护
static bool g_bSystemHasDpiScaling = false;

// 在程序初始化 (如 DllMain / main) 以及 WndProc 响应 WM_DISPLAYCHANGE / WM_DPICHANGED 时调用一次
void UpdateSystemDpiScalingStatus() {
	g_bSystemHasDpiScaling = false;
	EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMon, HDC, LPRECT, LPARAM pData) -> BOOL {
		UINT dpiX = 96, dpiY = 96;
		if (SUCCEEDED(GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
			if (dpiX != 96 || dpiY != 96) {
				*reinterpret_cast<bool*>(pData) = true;
				return FALSE;
			}
		}
		return TRUE;
		}, reinterpret_cast<LPARAM>(&g_bSystemHasDpiScaling));
}

// 精简后的 GetPhysicalRect
BOOL GetPhysicalRect(HWND hWnd, RECT* pRect) {
	if (!hWnd || !pRect) return FALSE;

	// 存在缩放 -> 优先 DWM；纯 100% 屏幕 -> 直接 GetWindowRect
	if (g_bSystemHasDpiScaling) {
		if (SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, pRect, sizeof(RECT)))) {
			return TRUE;
		}
	}

	return GetWindowRect(hWnd, pRect);
}

// 全局容器：记录被我们最小化的主屏窗口句柄
std::vector<HWND> g_minimizedPrimaryWindows;

// 全局变量：记录上一次点击右下角“显示桌面”的时间戳（单位：毫秒）
static ULONGLONG g_lastTrayClickTime = 0;

// 1. 用于打包传递给回调函数的轻量级上下文结构体
struct ToggleContext {
	std::vector<HWND>* pVector;
	HMONITOR hPrimaryMonitor; // 传入主显示器句柄
};

BOOL CALLBACK EnumWindowToggleProc(HWND hWnd, LPARAM lParam) {
	auto* pCtx = reinterpret_cast<ToggleContext*>(lParam);
	if (!pCtx || !pCtx->pVector || !IsValidAppWindow(hWnd)) {
		return TRUE;
	}

	// 1. 跳过最小化或不可见的窗口
	if (IsIconic(hWnd) || !IsWindowVisible(hWnd)) return TRUE;

	// 2. 核心过滤：如果外部已经赋予了 MTU_PROC_TAG 标签，直接跳过
	HANDLE hProc = GetPropW(hWnd, L"MTU_PROC_TAG");
	if (hProc != NULL) {
		return TRUE;
	}

	// 3. 跳过有 Owner 的子窗口/弹窗
	if (GetWindow(hWnd, GW_OWNER) != nullptr) return TRUE;

	// 4. 精准计算外框渲染物理矩形（剔除 Win10/11 阴影）
	RECT winRect;
	if (FAILED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &winRect, sizeof(winRect)))) {
		if (!GetPhysicalRect(hWnd, &winRect)) return TRUE;
	}

	// 5. 根据真实物理位置裁决所属显示器
	HMONITOR hWinMonitor = MonitorFromRect(&winRect, MONITOR_DEFAULTTONULL);

	// 6. 物理硬核防御：只收集严格位于主屏的窗口
	if (hWinMonitor == NULL || hWinMonitor != pCtx->hPrimaryMonitor) {
		return TRUE;
	}

	pCtx->pVector->push_back(hWnd);
	return TRUE;
}

// =========================================================================
// 2. 极致纯粹的全局低级鼠标钩子（状态完全依赖外部监测同步）
// =========================================================================
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0 && (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP)) {
		PMSLLHOOKSTRUCT pMouse = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);

		HWND hMainTray = FindWindowW(L"Shell_TrayWnd", NULL);
		if (hMainTray != NULL) {
			RECT trayRect;
			if (GetPhysicalRect(hMainTray, &trayRect)) {

				// 精准碰撞主屏任务栏右下角热区
				if (pMouse->pt.x >= (trayRect.right - TARGET_ZONE_WIDTH) && pMouse->pt.x <= trayRect.right &&
					pMouse->pt.y >= trayRect.top && pMouse->pt.y <= trayRect.bottom) {

					// 仅在按下时触发业务，抬起时静默拦截，彻底打断系统原生的 Show Desktop 消息链路
					if (wParam == WM_LBUTTONDOWN) {
						// 🟢 触发了显示桌面/隐藏操作，立即上锁！
						g_lastTrayClickTime = GetTickCount64();
						// 🟢 状态 A：历史记录为空 -> 执行“隐藏”
						if (g_minimizedPrimaryWindows.empty()) {
							POINT ptZero = { 0, 0 };
							HMONITOR hPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

							ToggleContext ctx = { &g_minimizedPrimaryWindows, hPrimary };

							// 1. 去主屏搜集【无 MTU_PROC_TAG】的窗口
							EnumWindows(EnumWindowToggleProc, reinterpret_cast<LPARAM>(&ctx));

							// 2. 批量异步最小化
							for (HWND hWnd : g_minimizedPrimaryWindows) {
								if (IsWindow(hWnd)) {
									PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
								}
							}
						}
						// 🟢 状态 B：历史记录不为空 -> 执行“恢复”
						else {
							for (HWND hWnd : g_minimizedPrimaryWindows) {
								// 确认句柄有效且当前仍处于最小化状态（避免误操作已被用户手动放大的窗口）
								if (IsWindow(hWnd) && IsIconic(hWnd)) {
									//RestoreWindowAppId(hWnd); // 恢复原始图标
									ShowWindowAsync(hWnd, SW_RESTORE); // 异步恢复，零阻塞
								}
							}
							g_minimizedPrimaryWindows.clear();
						}
					}

					// 🔴 完全吃掉该区域的 LBUTTONDOWN 与 LBUTTONUP 消息，阻止 Shell 响应
					return 1;
				}
			}
		}
	}
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// 辅助调试函数
#include <mutex>
#include <chrono>
#include <iomanip>

// 全局锁，确保多线程写入安全
std::mutex g_fileMutex;

// =========================================================================
// 完美兼容版：利用 std::wstring(var) 自动进行类型提升
// =========================================================================
#define LogTargetNameToFile(var) do {                                              \
    /* 将传入的参数强制包装为 std::wstring_view 或 std::wstring，兼容指针和对象 */  \
    std::wstring_view testStr(var);                                                \
    if (!testStr.empty()) {                                                        \
        /* 1. 自动获取当前时间戳 (精确到毫秒) */                                      \
        auto now = std::chrono::system_clock::now();                               \
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(           \
            now.time_since_epoch()) % 1000;                                        \
        auto timer = std::chrono::system_clock::to_time_t(now);                    \
        std::tm bt;                                                                \
        localtime_s(&bt, &timer);                                                  \
                                                                                   \
        /* 2. 多线程加锁保护 */                                                     \
        std::lock_guard<std::mutex> lock(g_fileMutex);                             \
                                                                                   \
        /* 3. 动态拼接宽字符文件名 */                                                \
        std::wstring fullPath = std::wstring(L## #var) + L".txt";                  \
        std::wofstream logFile(fullPath, std::ios::app);                           \
                                                                                   \
        if (logFile.is_open()) {                                                   \
            /* 设置区域以支持中文字符写入 */                                         \
            logFile.imbue(std::locale("chs"));                                     \
                                                                                   \
            /* 4. 写入格式: [2026-02-05 17:30:00.123] 变量值 */                       \
            logFile << L"[" << std::put_time(&bt, L"%F %T") << L"."                \
                    << std::setfill(L'0') << std::setw(3) << ms.count() << L"] "   \
                    << testStr << std::endl;                                       \
                                                                                   \
            logFile.close();                                                       \
        }                                                                          \
    }                                                                              \
} while(0)

// 辅助函数：通过 PID 获取进程名
std::wstring GetProcessNameByPid(DWORD pid) {
	if (pid == 0) return L"";

	// 优先尝试获取完整信息的权限，如果失败（如系统进程或保护进程），降级为仅查询信息权限
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!hProcess) {
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	}

	if (hProcess) {
		wchar_t buffer[MAX_PATH] = { 0 };
		DWORD size = MAX_PATH;
		// 使用 QueryFullProcessImageNameW 能极其稳定地拿到完整路径
		if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
			CloseHandle(hProcess);

			// 从完整路径中截取纯文件名（例如 C:\Windows\notepad.exe -> notepad.exe）
			std::wstring fullPath(buffer);
			size_t pos = fullPath.find_last_of(L"\\/");
			if (pos != std::wstring::npos) {
				return fullPath.substr(pos + 1);
			}
			return fullPath;
		}
		CloseHandle(hProcess);
	}
	return L"";
}

// =========================================================================
// 【核心新增】：高可靠性窗口显示状态判定函数（带微秒级自旋校验）
// =========================================================================
bool IsHiddenWindowReallyVisiable(HWND hwnd) {
	if (!IsWindow(hwnd)) return false;

	// 自旋重试 5 次，每次如果未就绪则让出 CPU 核心，给 Windows 样式刷新留出微秒级时间
	for (int i = 0; i < 5; ++i) {
		// 校验 1：标准可见性 API
		bool isVisible = IsWindowVisible(hwnd);

		// 校验 2：直接去内核读取窗口样式风格，看 WS_VISIBLE 位是否被强行注入
		LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
		bool hasVisibleStyle = (style & WS_VISIBLE) != 0;

		// 如果两层校验都通过，说明窗口是真的显示出来了
		if (isVisible && hasVisibleStyle) {
			return true;
		}

		// 如果还没变过来，说明 Windows 正在后台刷新样式，
		// 瞬间让出当前线程的 CPU 时间片，让系统内核快马加鞭去画窗口，避免占满 CPU 导致死循环
		SwitchToThread();
	}

	// 兜底判定：如果转了5圈样式还没刷新成功，为了绝对防止 while 变成死循环，
	// 我们再次检查 WS_VISIBLE 样式，只要样式在，就认为它已经走向显示流了
	LONG_PTR finalStyle = GetWindowLongPtrW(hwnd, GWL_STYLE);
	return (finalStyle & WS_VISIBLE) != 0;
}

void CALLBACK WinEventProc(HWINEVENTHOOK h, DWORD event, HWND hwnd, LONG idO, LONG idC, DWORD dwT, DWORD dwM) {
	// 基础过滤：只处理窗口对象
	if (hwnd == NULL || idO != OBJID_WINDOW) return;

	// 捕获插拔显示器、分辨率切换、桌面重建事件
	if (event == EVENT_SYSTEM_DESKTOPSWITCH) {
		UpdateSystemDpiScalingStatus(); // 刷新屏幕缩放标志位
		return;
	}

	//// =========================================================================
	//// 🔴【新增强制联动】：监控主屏事件变动，触发一键全屏还原大部队
	//// =========================================================================
	//// 注意：请确保 g_minimizedPrimaryWindows 属于全局变量（或在当前上下文中可达）
	//// 🔴 核心防御：如果距离上次点击右下角还不到 300ms，说明处于“显示桌面”的消息流中，直接跳过！
	//if (!g_minimizedPrimaryWindows.empty() && idC == CHILDID_SELF) {

	//	// 1. 仅保留前台焦点切换
	//	if (event == EVENT_SYSTEM_FOREGROUND)
	//	{
	//		if (IsWindow(hwnd)) {
	//			// 🎯 1. 过滤物理右键按下状态
	//			if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) {
	//				goto EXIT_RESTORE_CHECK;
	//			}

	//			// 🎯 2. 过滤右键菜单窗口（Windows 系统标准菜单类名 "#32768"）
	//			wchar_t className[256] = { 0 };
	//			GetClassNameW(hwnd, className, 256);
	//			if (wcscmp(className, L"#32768") == 0) {
	//				goto EXIT_RESTORE_CHECK; // 菜单本身获得焦点，忽略
	//			}

	//			// 🎯 3. 确认当前真正的前台窗口（Foreground Window）不是右键菜单
	//			HWND hFG = GetForegroundWindow();
	//			wchar_t fgClassName[256] = { 0 };
	//			if (hFG && IsWindow(hFG)) {
	//				GetClassNameW(hFG, fgClassName, 256);
	//				if (wcscmp(fgClassName, L"#32768") == 0) {
	//					goto EXIT_RESTORE_CHECK; // 菜单处于前台，忽略
	//				}
	//			}

	//			// 获取主显示器句柄
	//			POINT ptZero = { 0, 0 };
	//			HMONITOR hPrimaryMonitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

	//			bool isDesktop = (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0);
	//			bool isValidApp = IsValidAppWindow(hwnd);

	//			if (isValidApp || isDesktop) {

	//				HMONITOR hTargetMonitor = NULL;

	//				if (isDesktop) {
	//					// 获取事件产生那一刻的物理坐标
	//					DWORD pos = GetMessagePos();
	//					POINT ptEvent = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };

	//					hTargetMonitor = MonitorFromPoint(ptEvent, MONITOR_DEFAULTTONEAREST);
	//				}
	//				else {
	//					// 普通 App 窗口按其空间物理矩形裁决
	//					hTargetMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	//				}

	//				HMONITOR hFGMonitor = MonitorFromWindow(hFG, MONITOR_DEFAULTTONEAREST);

	//				// 确保触发目标和前台窗口均在主屏
	//				if (hTargetMonitor == hPrimaryMonitor && hFGMonitor == hPrimaryMonitor) {

	//					// 🔴 防误触发：300ms 冷却保护
	//					if (GetTickCount64() - g_lastTrayClickTime < 300) {
	//						goto EXIT_RESTORE_CHECK;
	//					}

	//					// 4. 批量拉回主屏幕所有最小化的窗口
	//					for (HWND hWnd : g_minimizedPrimaryWindows) {
	//						if (IsWindow(hWnd) && IsIconic(hWnd)) {
	//							ShowWindowAsync(hWnd, SW_RESTORE);
	//						}
	//					}

	//					// 5. 清空列表
	//					g_minimizedPrimaryWindows.clear();

	//					return;
	//				}
	//			}
	//		}
	//	}
	//}

	//EXIT_RESTORE_CHECK:;
	//// 🔽 这里下方可以继续放你其他的 WinEvent 事件监控代码（如 MINIMIZEEND、DESTROY 等）
	//// 这样其他监控逻辑 100% 正常运行，不受 300ms 冷却影响！

	// --- 1. 获取基础信息 ---
	wchar_t cls[256] = { 0 };
	wchar_t title[256] = { 0 };
	GetClassNameW(hwnd, cls, 256);
	GetWindowTextW(hwnd, title, 256);
	std::wstring className = cls;
	std::wstring windowTitle = title;

	// --- 1.5 继续尝试通过打开关联文件反隐藏之前的隐藏窗口 ---
	// 1. 获取当前的虚拟桌面名称与命令行纯文件名
	std::wstring forWhichPurpose = L"显示之前被隐藏的窗口";
	std::wstring currentVd = GetCurrentVirtualDesktopName(forWhichPurpose);

	if (((event == EVENT_OBJECT_SHOW || event == EVENT_OBJECT_CREATE ||
		event == EVENT_OBJECT_FOCUS || event == EVENT_SYSTEM_FOREGROUND)) && idC == CHILDID_SELF) {

		if (className == L"XamlExplorerHostIslandWindow") {
			return;
		}

		// 检查当前触发事件的窗口自身是否带有 MTU 属性标签
		HANDLE hSelfProcTag = GetPropW(hwnd, L"MTU_PROC_TAG");

		// 情况 A：新开一个 App 窗口 (满足前提 2)
		// 特征：触发了 SHOW 或 CREATE，且它是一个新窗口，绝对不应该带有标签。
		if (event == EVENT_OBJECT_SHOW || event == EVENT_OBJECT_CREATE) {
			if (hSelfProcTag != NULL) {
				return; // 兜底防御
			}
		}

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);

		// 排除管理工具自身的进程，只监控外部程序
		if (pid != 0 && pid != GetCurrentProcessId()) {
			std::wstring newProcName = GetProcessNameByPid(pid);

			if (newProcName == L"explorer.exe") {
				if (className != L"CabinetWClass") {
					newProcName.clear(); // 【成功修复】：清空字符串，替代非法的 NULL 赋值
				}
			}

			if (!newProcName.empty()) {

				// 【标记机制】(满足前提 3)：记录是否找到了因双击文件被外部暗中唤醒的特定隐藏老窗口
				HWND hSpecificWindow = NULL;
				if (hSelfProcTag != NULL && (event == EVENT_OBJECT_FOCUS || event == EVENT_SYSTEM_FOREGROUND)) {
					hSpecificWindow = hwnd;
				}

				// =========================================================================
				// 【一阶段 - 快照脱钩】：瞬间清理全局缓存 g_HiddenWindowsCache
				// =========================================================================
				std::vector<HWND> pMatchesToRestore;

				for (auto it = g_HiddenWindowsCache.begin(); it != g_HiddenWindowsCache.end(); /* 内部自增 */) {
					HWND hHidden = *it;
					bool isErased = false;

					if (IsWindow(hHidden)) {
						if (hSpecificWindow != NULL && hHidden == hSpecificWindow) {
							++it;
							continue;
						}

						HANDLE hProc = GetPropW(hHidden, L"MTU_PROC_TAG");
						HANDLE hVd = GetPropW(hHidden, L"MTU_VD_TAG");
						if (hProc && hVd) {
							wchar_t pName[MAX_PATH], vName[MAX_PATH];
							GlobalGetAtomNameW((ATOM)(UINT_PTR)hProc, pName, MAX_PATH);
							GlobalGetAtomNameW((ATOM)(UINT_PTR)hVd, vName, MAX_PATH);

							if (_wcsicmp(newProcName.c_str(), pName) == 0 && wcscmp(currentVd.c_str(), vName) == 0) {
								pMatchesToRestore.push_back(hHidden);
								it = g_HiddenWindowsCache.erase(it);
								isErased = true;
							}
						}
					}

					if (!isErased) {
						++it;
					}
				}

				// =========================================================================
				// 【二阶段 - 动态消费与显示状态断后校验】
				// =========================================================================
				while (!pMatchesToRestore.empty()) {
					// 1. 始终获取当前队列末尾的 hwnd 进行处理
					HWND hHidden = pMatchesToRestore.back();

					if (IsWindow(hHidden)) {

						// 只有当窗口带有标签时，才需要做这一整套反隐藏动作（防止死循环重复执行业务）
						if (GetPropW(hHidden, L"MTU_PROC_TAG") != NULL) {

							// --- A. 还原标题 (去掉前缀 🕸) ---
							wchar_t currentTitle[512] = { 0 };
							if (GetWindowTextW(hHidden, currentTitle, 512) > 0) {
								std::wstring t = currentTitle;
								std::wstring prefix = L"🕸";
								size_t pos = t.find(prefix);
								if (pos == 0) {
									t.erase(0, prefix.length());
									SetWindowTextW(hHidden, t.c_str());
								}
							}

							// --- B. 移除并清理 Atom 标签属性 ---
							HANDLE hProcRemove = RemovePropW(hHidden, L"MTU_PROC_TAG");
							if (hProcRemove) GlobalDeleteAtom((ATOM)(UINT_PTR)hProcRemove);

							HANDLE hVdRemove = RemovePropW(hHidden, L"MTU_VD_TAG");
							if (hVdRemove) GlobalDeleteAtom((ATOM)(UINT_PTR)hVdRemove);

							// --- C. 执行实际的反隐藏与置顶动作 ---
							ShowWindow(hHidden, SW_SHOW);
							SetForegroundWindow(hHidden);
							SetFocus(hHidden);
							SetWindowPos(hHidden, HWND_TOP, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);
						}
					}

					// =====================================================================
					// 【满足你的核心需求】：
					// 只有当窗口因为某些不可控外部原因意外关闭了 (!IsWindow)，
					// 或者我们自定义的 IsHiddenWindowReallyVisiable(hHidden) 明确返回 true（确保彻底脱离隐藏），
					// 才会执行 pop_back() 将其移出队列！
					// =====================================================================
					if (!IsWindow(hHidden) || IsHiddenWindowReallyVisiable(hHidden)) {
						pMatchesToRestore.pop_back(); // 顺利通关，安全弹出
					}
					// 如果不满足（即窗口还在隐藏中），由于没有 pop_back，
					// 下一次 while 循环会继续卡住它，但因为上面的 GetPropW 已经变成了 NULL，
					// 它不会重复去改标题和调 ShowWindow，而是安全地走入 IsHiddenWindowReallyVisiable 进行下一次等待校验。
				}

				// =========================================================================
				// 【二阶段顺利完成点】：此时 pMatchesToRestore 严格为空，大部队全部物理反隐藏成功！
				// =========================================================================

				// =========================================================================
				// 【三阶段 - 延后处理特定窗口】
				// =========================================================================
				if (hSpecificWindow != NULL && IsWindow(hSpecificWindow)) {

					HANDLE hProc = GetPropW(hSpecificWindow, L"MTU_PROC_TAG");
					HANDLE hVd = GetPropW(hSpecificWindow, L"MTU_VD_TAG");
					if (hProc && hVd) {
						wchar_t pName[MAX_PATH], vName[MAX_PATH];
						GlobalGetAtomNameW((ATOM)(UINT_PTR)hProc, pName, MAX_PATH);
						GlobalGetAtomNameW((ATOM)(UINT_PTR)hVd, vName, MAX_PATH);

						if (_wcsicmp(newProcName.c_str(), pName) == 0 && wcscmp(currentVd.c_str(), vName) == 0) {

							for (auto it = g_HiddenWindowsCache.begin(); it != g_HiddenWindowsCache.end(); ++it) {
								if (*it == hSpecificWindow) {
									g_HiddenWindowsCache.erase(it);
									break;
								}
							}

							wchar_t currentTitle[512] = { 0 };
							if (GetWindowTextW(hSpecificWindow, currentTitle, 512) > 0) {
								std::wstring t = currentTitle;
								std::wstring prefix = L"🕸";
								size_t pos = t.find(prefix);
								if (pos == 0) {
									t.erase(0, prefix.length());
									SetWindowTextW(hSpecificWindow, t.c_str());
								}
							}

							HANDLE hProcRemove = RemovePropW(hSpecificWindow, L"MTU_PROC_TAG");
							if (hProcRemove) GlobalDeleteAtom((ATOM)(UINT_PTR)hProcRemove);

							HANDLE hVdRemove = RemovePropW(hSpecificWindow, L"MTU_VD_TAG");
							if (hVdRemove) GlobalDeleteAtom((ATOM)(UINT_PTR)hVdRemove);

							ShowWindow(hSpecificWindow, SW_SHOW);
							SetForegroundWindow(hSpecificWindow);
							SetFocus(hSpecificWindow);
							SetWindowPos(hSpecificWindow, HWND_TOP, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);
						}
					}
				} // 结束特定窗口处理

			}
		}
	}

	// --- 2. 最小化/恢复监控 (仅针对窗口自身 idC == CHILDID_SELF) ---
	if (idC == CHILDID_SELF) {
		// 最小化开始：触发 AppID 独立化
		if (event == EVENT_SYSTEM_MINIMIZESTART) {

			//// A. 排除 Win+D / Win+M 群发效应
			//static ULONGLONG lastTime = 0;
			//static int batchCount = 0;
			//ULONGLONG now = GetTickCount64();
			//if (now - lastTime < 100) batchCount++;
			//else batchCount = 1;
			//lastTime = now;
			//if (batchCount > 1) return;

			// A.检查当前前台窗口是否为任务栏或桌面
			HWND hForeground = GetForegroundWindow();
			wchar_t className[256];
			GetClassNameW(hForeground, className, 256);
			// 如果前台窗口已经是 Progman (监控Win+M) 或 Shell_TrayWnd (监控右下角触发)
			// 说明 Win+D 的动作流已经启动
			if (wcscmp(className, L"Progman") == 0) {
				// 检查键盘状态：如果 Win 键没被按下，说明不是 Win+D/M 触发的批量动作 
				if (GetAsyncKeyState(VK_LWIN) & 0x8000) {
					// Win键被触发，允许继续执行 AppID 独立化逻辑
					return; // 确实是 Win+M/D，拦截
				}
			}

			// B. 排除点击任务栏导致的最小化
			POINT pt;
			GetCursorPos(&pt);
			HWND hUnderMouse = WindowFromPoint(pt);
			wchar_t trayCls[256];
			GetClassNameW(hUnderMouse, trayCls, 256);
			std::wstring mouseOnClass = trayCls;
			if (wcscmp(trayCls, L"Shell_TrayWnd") == 0) {
				RECT trayRect;
				GetPhysicalRect(hUnderMouse, &trayRect);
				// 如果点击位置不在任务栏的最右侧边缘（通常右下角触发区很窄）
				// 则认为是普通的点击任务栏图标最小化，可以选择 return
				if (pt.x < trayRect.right - 15) {
					return;
				}
				// 如果是在最右侧，说明是“显示桌面”，我们【不 return】，继续执行 AppID 分离
			}
			if (wcscmp(trayCls, L"Shell_SecondaryTrayWnd") == 0) {
				return;
			}

			// C. 执行 AppID 分离逻辑
			std::wstring uniqueId = L"MTU.AppSplit." + std::to_wstring((UINT_PTR)hwnd);
			SetWindowAppIdWithBackup(hwnd, uniqueId.c_str());
			return;
		}
		//显式向 Shell 申请抹掉小图标：
		if (event == EVENT_SYSTEM_MINIMIZEEND) {
			RestoreWindowAppId(hwnd);

			ITaskbarList3* pTL = nullptr;
			if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTL)))) {
				pTL->HrInit();

#pragma warning(push)
#pragma warning(disable: 6387) // 暂时抑制 C6387 警告
				pTL->SetOverlayIcon(hwnd, nullptr, L"");
#pragma warning(pop)

				pTL->Release();
			}

			SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
			return;
		}
	}

	// --- 3. 全局压制逻辑 ---
	// 只要窗口显示且不在任务视图中，就检查是否持有标记
	SuppressHiddenWindows(hwnd, event);
	// --- 4. 任务视图监控：单次触发模式 ---
	bool isTaskViewWindow = (className == L"XamlExplorerHostIslandWindow" && windowTitle == L"任务视图");
	if (isTaskViewWindow) {
		// 1. 尺寸过滤：排除鼠标悬停产生的微型预览窗口（Win10/11 悬停预览通常小于 500px）
		RECT rc;
		GetPhysicalRect(hwnd, &rc);
		if ((rc.right - rc.left) < 500) return;
		// 2. 识别“进入”或“在其中切换”的瞬间
		// EVENT_OBJECT_SHOW: 任务视图刚打开
		// EVENT_SYSTEM_FOREGROUND: 焦点切回任务视图
		if (event == EVENT_OBJECT_SHOW || event == EVENT_SYSTEM_FOREGROUND) {
			// 3. 状态锁：确保在一次任务视图生命周期内只执行一次转移逻辑
			if (!g_InSnapWindows) {
				g_InSnapWindows = true;

				// --- 执行 TODO：窗口转移逻辑 ---
				// 这里调用你之后要实现的函数
				// ExecuteWindowTransferByRules(); 

				ShowWin11NativeNotification(L"待完成", L"Win+Tab可完成窗口转移");
			}
		}
		if (event == EVENT_OBJECT_DESTROY) {
			//退出任务视图的时候应该还是可以做点什么
		}
	}
	else {
		// 4. 关键：当焦点不再是任务视图窗口时，重置触发锁
		// 这样下次用户再次按下 Win+Tab 时，逻辑可以再次触发
		if (event == EVENT_SYSTEM_FOREGROUND) {
			g_InSnapWindows = false;
		}
	}
}

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrev, _In_ LPSTR lpCmd, _In_ int nShow) {
	// =========================================================================
	// 【核心新增】：使用唯一的命名 Mutex 实现单实例检测
	// =========================================================================
	// 名字可以自己换，加 L"Global\\" 前缀可以确保即使在多用户切换或管理员权限下也是全局唯一的
	HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\MyWindowManagerUniqueMutex_12345");

	// 如果创建失败，或者系统返回 ERROR_ALREADY_EXISTS，说明已有实例在运行
	if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		if (hMutex) {
			CloseHandle(hMutex); // 安全释放句柄
		}
		// 发现多开，直接静默退出（也可以在此处加一个 MessageBoxW 弹窗提示用户）
		return 0;
	}
	// =========================================================================

	// 1. 初始化 WinRT 环境 (用于原生通知)
	winrt::init_apartment();
	// 2. 注册应用快捷方式
	RegisterAppShortcut();
	// 3. 挂载低级别钩子
	g_hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyProc, hInst, 0);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInst, 0);
	//if (!g_hMouseHook) {
	//	return 1;
	//}

	g_hSysHook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_MINIMIZEEND,
		NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);
	// Hook 2: 监控对象显示 (0x8002)
	g_hObjHook = SetWinEventHook(
		EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
		NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);

	//监控屏幕切换
	HWINEVENTHOOK g_hDesktopHook = NULL; // 注意类型是 HWINEVENTHOOK，不是 HHOOK

	g_hDesktopHook = SetWinEventHook(
		EVENT_SYSTEM_DESKTOPSWITCH, EVENT_SYSTEM_DESKTOPSWITCH,
		NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);

	if (g_hDesktopHook) UnhookWinEvent(g_hDesktopHook);

	// 7. 标准消息循环 (驱动 WinEventProc 的核心)
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	// 8. 退出清理 (仅清理本进程的事件钩子)
	if (g_hKeyHook) UnhookWindowsHookEx(g_hKeyHook);
	if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
	if (g_hSysHook) UnhookWinEvent(g_hSysHook);
	if (g_hObjHook) UnhookWinEvent(g_hObjHook);

	// =========================================================================
	// 【核心新增】：程序退出时释放并关闭互斥量句柄
	// =========================================================================
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	// 【核心改造】程序退出前，优雅释放、销毁当前主线程的 COM 环境
	winrt::uninit_apartment();

	return 0;
}