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
#include <oleacc.h>
#include <filesystem>
#include <unordered_map>
#include <iterator>
#include <algorithm>

// WinRT 通知相关
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
// 链接库
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "oleacc.lib")
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
static constexpr wchar_t PROP_ORIGINAL_APPID[] = L"MTU_ORIGINAL_APPID";

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
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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

// Desktop membership is an identity, not a user-editable desktop name.
// We store the official GUID as a string Atom in MTU_VD_TAG. Existing name
// tags intentionally fail closed: an unrecognized desktop is never unsealed.
static bool GetWindowDesktopGuid(HWND h, GUID& id, bool requireCurrent = false) {
    if (!IsWindow(h)) return false;
    IVirtualDesktopManager* manager = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&manager));
    if (FAILED(hr) || !manager) return false;
    BOOL onCurrent = FALSE;
    if (requireCurrent)
        hr = manager->IsWindowOnCurrentVirtualDesktop(h, &onCurrent);
    if (SUCCEEDED(hr) && (!requireCurrent || onCurrent))
        hr = manager->GetWindowDesktopId(h, &id);
    manager->Release();
    return SUCCEEDED(hr) && (!requireCurrent || onCurrent) && !IsEqualGUID(id, GUID{});
}

static bool ReadTaggedDesktopGuid(HWND h, GUID& id) {
    const auto atom = reinterpret_cast<UINT_PTR>(GetPropW(h, L"MTU_VD_TAG"));
    wchar_t value[64]{};
    if (atom < MAXINTATOM || atom > 0xffff ||
        !GlobalGetAtomNameW(static_cast<ATOM>(atom), value, static_cast<int>(std::size(value))))
        return false;
    return wcslen(value) == 38 && value[0] == L'{' && value[37] == L'}' &&
        SUCCEEDED(CLSIDFromString(value, &id)) && !IsEqualGUID(id, GUID{});
}

static bool TaggedDesktopMatches(HWND h, const GUID& expected) {
    GUID tag{};
    return ReadTaggedDesktopGuid(h, tag) && IsEqualGUID(tag, expected);
}

static constexpr wchar_t SHARED_UWP_HOST[] = L"ApplicationFrameHost.exe";

static std::wstring ReadTaggedExe(HWND h) {
    const auto tag = GetPropW(h, L"MTU_EXE_TAG");
    wchar_t process[MAX_PATH]{};
    const auto atom = reinterpret_cast<UINT_PTR>(tag);
    if (atom < MAXINTATOM || atom > 0xffff ||
        !GlobalGetAtomNameW(static_cast<ATOM>(atom), process, static_cast<int>(std::size(process)))) return {};
    return process;
}

static bool IsIdentifiedProcess(const std::wstring& process) {
    return !process.empty() && _wcsicmp(process.c_str(), L"Unknown") != 0 &&
        _wcsicmp(process.c_str(), SHARED_UWP_HOST) != 0;
}

using ReadWindowDesktop = bool(*)(HWND, GUID&, bool);

static bool CanUnsealOnCurrentDesktop(HWND h, ReadWindowDesktop readDesktop = GetWindowDesktopGuid) {
    if (!IsIdentifiedProcess(ReadTaggedExe(h))) return false;
    GUID current{};
    return readDesktop(h, current, true) && TaggedDesktopMatches(h, current);
}

// 事件监控相关
// --- 全局配置与状态 ---
static constexpr UINT_PTR TIMER_ID_COLLECT = 1001;
bool g_InSnapWindows = false;
static ULONGLONG g_altTabNextCollectAt = 0;
static ULONGLONG g_altTabCollectDeadline = 0;

static void CancelAltTabCollectTimer(HWND hwnd) {
    if (hwnd) KillTimer(hwnd, TIMER_ID_COLLECT);
    g_altTabNextCollectAt = 0;
    g_altTabCollectDeadline = 0;
}
// 全局句柄
HHOOK g_hKeyHook = NULL;
HHOOK g_hMouseHook = NULL;
HWINEVENTHOOK g_hSysHook = NULL;
HWINEVENTHOOK g_hObjHook = NULL;

// 建议：句柄缓存 - 避免频繁 EnumWindows 带来的性能开销
std::vector<HWND> g_HiddenWindowsCache;

// Preserve original presentation across seals and Alt+Tab previews.
static constexpr UINT WINDOW_FRAME_FLAGS =
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER;

static bool IsApplicationFrameWindow(HWND h) {
    wchar_t name[32]{};
    return GetClassNameW(h, name, static_cast<int>(std::size(name))) &&
        wcscmp(name, L"ApplicationFrameWindow") == 0;
}

static bool RequestHostedUwpClose(HWND h) {
    return IsApplicationFrameWindow(h) && PostMessageW(h, WM_CLOSE, 0, 0) != FALSE;
}

static std::unordered_map<HWND, ULONGLONG> g_restoredWindowShows;
static void RestoreAllPrimaryWindowTransparency();

static bool SetWindowPresented(HWND h, bool visible, UINT flags = 0) {
    if (visible) {
        const auto now = GetTickCount64();
        std::erase_if(g_restoredWindowShows, [now](const auto& event) {
            return now - event.second > 3000;
        });
        g_restoredWindowShows[h] = now;
    }
    if (SetWindowPos(h, nullptr, 0, 0, 0, 0,
            WINDOW_FRAME_FLAGS | flags | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW))) return true;
    if (visible) g_restoredWindowShows.erase(h);
    return false;
}

struct SealedWindowState {
    LONG_PTR exStyle = 0;
    BYTE alpha = 255;
    COLORREF colorKey = 0;
    DWORD layerFlags = 0;
    bool hasLayerAttributes = false;
    bool wasVisible = false;
    bool wasIconic = false;
    bool hiddenByLasso = false;
};
static std::unordered_map<HWND, SealedWindowState> g_sealedWindowStates;

static bool SnapshotWindowState(HWND h) {
    if (!IsWindow(h) || IsApplicationFrameWindow(h)) return false;
    if (g_sealedWindowStates.contains(h)) return true;
    SealedWindowState state;
    state.exStyle = GetWindowLongPtrW(h, GWL_EXSTYLE);
    state.wasVisible = IsWindowVisible(h) != FALSE;
    state.wasIconic = IsIconic(h) != FALSE;
    // If an existing layered window does not expose its original attributes,
    // leave it alone rather than rewriting its alpha and losing its content.
    if (state.exStyle & WS_EX_LAYERED) {
        if (!GetLayeredWindowAttributes(h, &state.colorKey, &state.alpha, &state.layerFlags))
            return false;
        state.hasLayerAttributes = true;
    }
    g_sealedWindowStates.emplace(h, state);
    return true;
}

static void ClearSealMarkers(HWND h) {
    HANDLE p = RemovePropW(h, L"MTU_EXE_TAG");
    if (p) GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(p)));
    HANDLE vd = RemovePropW(h, L"MTU_VD_TAG");
    if (vd) GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(vd)));
    wchar_t title[512]{};
    if (GetWindowTextW(h, title, 512) > 0) {
        const std::wstring prefix = L"🕸";
        if (wcsncmp(title, prefix.c_str(), prefix.length()) == 0)
            SetWindowTextW(h, title + prefix.length());
    }
}

static bool IsOriginalWindowEligibleForPreview(HWND h) {
    if (!IsWindow(h)) return false;
    const auto it = g_sealedWindowStates.find(h);
    return it != g_sealedWindowStates.end() && it->second.wasVisible &&
        !(it->second.exStyle & WS_EX_TOOLWINDOW) &&
        !it->second.wasIconic && !IsIconic(h) &&
        (it->second.hiddenByLasso || IsWindowVisible(h));
}

static void UpdateTaskbarButton(HWND h, bool visible) {
    winrt::com_ptr<ITaskbarList> taskbar;
    if (FAILED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
        IID_ITaskbarList, taskbar.put_void())) || FAILED(taskbar->HrInit())) return;
    if (visible) taskbar->AddTab(h);
    else taskbar->DeleteTab(h);
}

// Keep the snapshot and tags during the temporary native switcher preview.
bool ShowWindowAndShowTaskbarIcon(HWND h, bool preview = false) {
    auto it = g_sealedWindowStates.find(h);
    if (it == g_sealedWindowStates.end()) return false;
    if (!IsWindow(h)) { g_sealedWindowStates.erase(it); return false; }

    auto& original = it->second;
    if (preview && !IsOriginalWindowEligibleForPreview(h)) return false;

    // Like the original switcher path, temporarily make the preview an
    // app window and explicitly show it. The snapshot owns the final style.
    const LONG_PTR targetStyle = preview ? original.exStyle | WS_EX_APPWINDOW : original.exStyle;
    if (preview && IsWindowVisible(h) && !SetWindowPresented(h, false)) return false;
    if (preview) original.hiddenByLasso = true;

    SetLastError(ERROR_SUCCESS);
    if (!SetWindowLongPtrW(h, GWL_EXSTYLE, targetStyle) && GetLastError() != ERROR_SUCCESS)
        return false;
    if (original.hasLayerAttributes &&
        !SetLayeredWindowAttributes(h, original.colorKey, original.alpha, original.layerFlags))
        return false;
    if (preview || original.hiddenByLasso) {
        if (!SetWindowPresented(h, true, SWP_FRAMECHANGED) || !IsWindowVisible(h)) return false;
        original.hiddenByLasso = false;
    } else if (!SetWindowPos(h, nullptr, 0, 0, 0, 0, WINDOW_FRAME_FLAGS | SWP_FRAMECHANGED)) {
        return false;
    }
    if (original.wasVisible && IsWindowVisible(h) && !(targetStyle & WS_EX_TOOLWINDOW))
        UpdateTaskbarButton(h, true);
    InvalidateRect(h, nullptr, TRUE);
    if (!preview) {
        ClearSealMarkers(h);
        g_sealedWindowStates.erase(it);
    }
    return true;
}

bool HideWindowAndHideTaskbarIcon(HWND h, bool afterPreview = false) {
    if (!SnapshotWindowState(h)) return false;
    auto& original = g_sealedWindowStates.at(h);
    LONG_PTR exStyle = GetWindowLongPtrW(h, GWL_EXSTYLE);
    // Give Explorer the same visibility transition on re-seal as on preview.
    // Do it only for a live preview so the initial cross-desktop seal never shows a window.
    if (afterPreview && IsWindowVisible(h) && !IsIconic(h) && !(exStyle & WS_EX_TOOLWINDOW) &&
        SetWindowPresented(h, false)) original.hiddenByLasso = true;

    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT;
    SetLastError(ERROR_SUCCESS);
    const bool styleChanged =
        SetWindowLongPtrW(h, GWL_EXSTYLE, exStyle) != 0 || GetLastError() == ERROR_SUCCESS;
    const bool transparent = styleChanged && SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
    if (styleChanged)
        SetWindowPos(h, nullptr, 0, 0, 0, 0, WINDOW_FRAME_FLAGS | SWP_FRAMECHANGED);

    if (afterPreview && original.hiddenByLasso) {
        if (!IsWindowVisible(h) && !SetWindowPresented(h, true)) return false;
        original.hiddenByLasso = false;
    }
    if (!transparent) return false;
    UpdateTaskbarButton(h, false);
    return true;
}

static void RestoreAllSealedWindows() {
    // This shared cleanup path also runs during shutdown. Do not couple it to
    // temporary primary transparency; only explicit user input restores that state.
    for (auto it = g_sealedWindowStates.begin(); it != g_sealedWindowStates.end();) {
        const HWND h = (it++)->first;
        ShowWindowAndShowTaskbarIcon(h);
    }
}

// --- 逻辑 1：全局压制逻辑 ---
void SuppressHiddenWindows(HWND hwnd, DWORD event) {
	// 只有在非任务视图模式下才执行压制
	if (event == EVENT_OBJECT_SHOW && !g_InSnapWindows && CanUnsealOnCurrentDesktop(hwnd)) {
		// 关键：只要 GetPropW 返回非空，说明窗口已被标记，直接压制
		if (GetPropW(hwnd, L"MTU_EXE_TAG")) {
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
		if (GetPropW(h, L"MTU_EXE_TAG")) {
			BOOL isCurrent = FALSE;

			// --- 核心判定：由系统告诉你这个窗口是否在当前活跃桌面 ---
			if (SUCCEEDED(pManager->IsWindowOnCurrentVirtualDesktop(h, &isCurrent))) {
				if (isCurrent) {
					// 在当前桌面，加入待管理缓存
					g_HiddenWindowsCache.push_back(h);
				}
				// Foreign-desktop windows are never modified while refreshing cache.
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

static bool IsSwitcherWindow(HWND h) {
    wchar_t name[96]{};
    if (!GetClassNameW(h, name, static_cast<int>(std::size(name)))) return false;
    constexpr wchar_t simpleSwitcher[] = L"SimpleWindowSwitcher_";
    return wcscmp(name, L"XamlExplorerHostIslandWindow") == 0 ||
        wcscmp(name, L"MultitaskingViewFrame") == 0 ||
        wcscmp(name, L"TaskSwitcherWnd") == 0 ||
        wcscmp(name, L"TaskSwitcherOverlayWnd") == 0 ||
        wcsncmp(name, simpleSwitcher, std::size(simpleSwitcher) - 1) == 0;
}

static bool ReadDesktopRegistryGuid(const wchar_t* key, const wchar_t* value, GUID& desktop) {
    GUID id{};
    DWORD size = sizeof(id);
    if (RegGetValueW(HKEY_CURRENT_USER, key, value, RRF_RT_REG_BINARY,
            nullptr, &id, &size) != ERROR_SUCCESS || size != sizeof(id) ||
        IsEqualGUID(id, GUID{})) return false;
    desktop = id;
    return true;
}

static DWORD GetCurrentSessionId() {
    DWORD session = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &session);
    return session;
}

using ReadDesktopRegistry = bool(*)(const wchar_t*, const wchar_t*, GUID&);

static bool ReadCurrentDesktopGuidFromRegistry(
    GUID& desktop,
    ReadDesktopRegistry readRegistry = ReadDesktopRegistryGuid,
    DWORD session = GetCurrentSessionId()) {
    constexpr wchar_t key[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";

    // The per-session value is authoritative for the interactive session.
    // The unscoped value can lag behind after a virtual-desktop switch.
    if (session != 0) {
        wchar_t sessionKey[160]{};
        swprintf_s(sessionKey,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo\\%lu\\VirtualDesktops",
            session);
        if (readRegistry(sessionKey, L"CurrentVirtualDesktop", desktop)) return true;
    }

    if (readRegistry(key, L"CurrentVirtualDesktop", desktop)) return true;
    // VirtualDesktopIDs is a single GUID only when the user has one desktop.
    return readRegistry(key, L"VirtualDesktopIDs", desktop);
}

static bool GetCurrentDesktopGuidFromForeground(
    GUID& desktop,
    HWND anchor,
    ReadWindowDesktop readDesktop = GetWindowDesktopGuid,
    ReadDesktopRegistry readRegistry = ReadDesktopRegistryGuid,
    DWORD session = GetCurrentSessionId()) {
    // Prefer a regular foreground window when Windows can confirm its desktop.
    const HWND foreground = anchor ? GetAncestor(anchor, GA_ROOT) : nullptr;
    wchar_t className[64]{};
    constexpr GUID pinnedApp{0xbb64d5b7, 0x4de3, 0x4ab2, {0xa8, 0x7c, 0xdb, 0x76, 0x01, 0xae, 0xa7, 0xdc}};
    constexpr GUID pinnedWindow{0xc2ddea68, 0x66f2, 0x4cf9, {0x82, 0x64, 0x1b, 0xfd, 0x00, 0xfb, 0xbb, 0xac}};
    GUID id{};
    if (foreground && IsWindowVisible(foreground) && !IsSwitcherWindow(foreground) &&
        GetClassNameW(foreground, className, static_cast<int>(std::size(className))) &&
        wcscmp(className, L"Progman") != 0 && wcscmp(className, L"WorkerW") != 0 &&
        wcscmp(className, L"Shell_TrayWnd") != 0 && wcscmp(className, L"Shell_SecondaryTrayWnd") != 0 &&
        readDesktop(foreground, id, true) &&
        !IsEqualGUID(id, pinnedApp) && !IsEqualGUID(id, pinnedWindow)) {
        desktop = id;
        return true;
    }

    // When every app window is sealed, the foreground may be the Shell.
    // A sealed window can identify the active desktop if the public API still
    // reports it as current; otherwise use the session-scoped Explorer value.
    for (const auto& entry : g_sealedWindowStates) {
        const HWND window = entry.first;
        GUID tagged{};
        if (!ReadTaggedDesktopGuid(window, tagged) || !IsIdentifiedProcess(ReadTaggedExe(window)))
            continue;
        if (readDesktop(window, id, true) && IsEqualGUID(id, tagged) &&
            !IsEqualGUID(id, pinnedApp) && !IsEqualGUID(id, pinnedWindow)) {
            desktop = id;
            return true;
        }
    }

    return ReadCurrentDesktopGuidFromRegistry(desktop, readRegistry, session);
}

static bool GetCurrentDesktopGuid(GUID& desktop) {
    return GetCurrentDesktopGuidFromForeground(desktop, GetForegroundWindow());
}

using ReadCurrentDesktop = bool(*)(GUID&);
static void FinishAltTabPreview(HWND winner, ReadCurrentDesktop readCurrentDesktop = GetCurrentDesktopGuid);
static void OnCollectTimer(HWND hwnd, UINT_PTR idEvent) {
    // KillTimer does not remove WM_TIMER messages already in the queue.
    // Ignore them until the current Alt release has actually armed collection.
    const auto now = GetTickCount64();
    if (!g_InSnapWindows || !g_altTabNextCollectAt || now < g_altTabNextCollectAt) return;

    HWND foreground = GetForegroundWindow();
    const bool switcherActive = IsSwitcherWindow(foreground);
    if (switcherActive && now < g_altTabCollectDeadline &&
        SetTimer(hwnd, idEvent, 100, nullptr)) {
        g_altTabNextCollectAt = now + 100;
        return;
    }
    CancelAltTabCollectTimer(hwnd);
    FinishAltTabPreview(switcherActive ? nullptr : foreground);
}

//realname一致的同类窗口处理
struct CloseFilter {
    std::wstring currentAppProcName;
    ReadWindowDesktop readDesktop = GetWindowDesktopGuid;
};

static bool SetSealTag(HWND h, const wchar_t* property, const wchar_t* value) {
    const auto old = GetPropW(h, property);
    const ATOM atom = GlobalAddAtomW(value);
    if (!atom) return false;
    if (!SetPropW(h, property, reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(atom)))) {
        GlobalDeleteAtom(atom);
        return false;
    }
    if (old) GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(old)));
    return true;
}

static bool SealWindow(HWND h, const std::wstring& process, REFGUID desktop) {
    wchar_t desktopText[40]{};
    if (process.empty() || IsEqualGUID(desktop, GUID{}) ||
        !StringFromGUID2(desktop, desktopText, static_cast<int>(std::size(desktopText)))) return false;
    const bool tracked = g_sealedWindowStates.contains(h);
    if (!SnapshotWindowState(h)) return false;

    const bool hadProcess = GetPropW(h, L"MTU_EXE_TAG") != nullptr;
    const bool processMatches = _wcsicmp(ReadTaggedExe(h).c_str(), process.c_str()) == 0;
    if ((!processMatches && !SetSealTag(h, L"MTU_EXE_TAG", process.c_str())) ||
        (!TaggedDesktopMatches(h, desktop) && !SetSealTag(h, L"MTU_VD_TAG", desktopText))) {
        if (!hadProcess) {
            if (const auto added = RemovePropW(h, L"MTU_EXE_TAG"))
                GlobalDeleteAtom(static_cast<ATOM>(reinterpret_cast<UINT_PTR>(added)));
        }
        if (!tracked) g_sealedWindowStates.erase(h);
        return false;
    }
    if (!HideWindowAndHideTaskbarIcon(h)) {
        ShowWindowAndShowTaskbarIcon(h);
        return false;
    }

    wchar_t title[512]{};
    const std::wstring prefix = L"🕸";
    if (GetWindowTextW(h, title, static_cast<int>(std::size(title))) > 0 &&
        wcsncmp(title, prefix.c_str(), prefix.length()) != 0)
        SetWindowTextW(h, (prefix + title).c_str());
    return true;
}

struct HideCurrentFilter {
    std::wstring process;
    ReadWindowDesktop readDesktop = GetWindowDesktopGuid;
};

BOOL CALLBACK EnumHideWindowsProc(HWND h, LPARAM lParam) {
    const auto* filter = reinterpret_cast<const HideCurrentFilter*>(lParam);
    if (!filter) return FALSE;
    if (!IsWindowVisible(h) || GetParent(h) != nullptr) return TRUE;
    if (_wcsicmp(GetRealProcessName(h).c_str(), filter->process.c_str()) != 0) return TRUE;

    wchar_t className[256]{};
    if (_wcsicmp(filter->process.c_str(), L"explorer.exe") == 0 &&
        (!GetClassNameW(h, className, static_cast<int>(std::size(className))) ||
         wcscmp(className, L"CabinetWClass") != 0)) return TRUE;

    if (IsApplicationFrameWindow(h)) {
        RequestHostedUwpClose(h);
        return TRUE;
    }

    // Use this window's desktop even when it differs from the foreground desktop.
    GUID desktop{};
    if (filter->readDesktop(h, desktop, false)) SealWindow(h, filter->process, desktop);
    return TRUE;
}

// 反向隐藏：隐藏所有【不属于】当前 App 进程名的窗口
BOOL CALLBACK EnumHideOthersProc(HWND hWnd, LPARAM lParam) {
    auto* filter = reinterpret_cast<CloseFilter*>(lParam);
    if (!filter) return FALSE;
    if (!IsWindowVisible(hWnd) || GetParent(hWnd) != nullptr) return TRUE;

    const std::wstring otherWndProcName = GetRealProcessName(hWnd);
    if (_wcsicmp(otherWndProcName.c_str(), filter->currentAppProcName.c_str()) == 0)
        return TRUE;

    // UWP frames use the conservative WM_CLOSE path regardless of desktop.
    if (IsApplicationFrameWindow(hWnd)) {
        if (filter->currentAppProcName != SHARED_UWP_HOST &&
            otherWndProcName != L"Unknown" &&
            _wcsicmp(otherWndProcName.c_str(), SHARED_UWP_HOST) != 0)
            RequestHostedUwpClose(hWnd);
        return TRUE;
    }

    // Keep the Explorer safety rule: only File Explorer windows are eligible.
    if (_wcsicmp(otherWndProcName.c_str(), L"explorer.exe") == 0) {
        wchar_t className[256]{};
        if (!GetClassNameW(hWnd, className, static_cast<int>(std::size(className))) ||
            wcscmp(className, L"CabinetWClass") != 0)
            return TRUE;
    }

    // Do not require the current desktop. Tag each window with its own GUID.
    GUID desktop{};
    if (filter->readDesktop(hWnd, desktop, false))
        SealWindow(hWnd, otherWndProcName, desktop);
    return TRUE;
}


// 隐藏同类型窗口后，alt+tab临时查看下。
// --- 全局变量与状态控制 ---
static GUID g_altTabPreviewDesktop{};
std::vector<HWND> g_snapUnhiddenWindows; // 存储 Alt+Tab 临时反隐藏的窗口句柄列表
// ============================================================================
// 1. 现代按键模拟辅助函数 (基于 SendInput)
// ============================================================================

// Replay the chord even when Alt or Shift was released while Tab was queued.
static size_t BuildAltTabInput(INPUT (&inputs)[6], bool altDown, bool shiftDown, bool reverse) {
    size_t count = 0;
    auto append = [&](WORD key, DWORD flags = 0) {
        INPUT& input = inputs[count++];
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key;
        input.ki.dwFlags = flags;
    };
    const bool synthesizeShift = reverse && !shiftDown;
    if (!altDown) append(VK_MENU);
    if (synthesizeShift) append(VK_SHIFT);
    append(VK_TAB);
    append(VK_TAB, KEYEVENTF_KEYUP);
    if (synthesizeShift) append(VK_SHIFT, KEYEVENTF_KEYUP);
    if (!altDown) append(VK_MENU, KEYEVENTF_KEYUP);
    return count;
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
// Alt+Tab 临时预览及同桌面进程组收网
// ============================================================================

bool TryUnHideWindowsForAltTab(ReadCurrentDesktop readCurrentDesktop = nullptr,
                              HWND foreground = GetForegroundWindow()) {
    RestoreAllPrimaryWindowTransparency();
    // Repeated Tab advances the native switcher without rebuilding the preview.
    if (g_InSnapWindows) return true;
    if (g_sealedWindowStates.empty()) return false;
    GUID currentDesktop{};
    const bool resolved = readCurrentDesktop ? readCurrentDesktop(currentDesktop)
        : GetCurrentDesktopGuidFromForeground(currentDesktop, foreground);
    if (!resolved || IsEqualGUID(currentDesktop, GUID{})) return false;
    g_altTabPreviewDesktop = currentDesktop;
    std::vector<HWND> candidates;
    for (const auto& entry : g_sealedWindowStates) candidates.push_back(entry.first);

    // Show events generated by the preview must not immediately re-seal it.
    g_InSnapWindows = true;
    for (HWND h : candidates) {
        if (!IsOriginalWindowEligibleForPreview(h) ||
            !TaggedDesktopMatches(h, currentDesktop) ||
            !IsIdentifiedProcess(ReadTaggedExe(h))) continue;

        // A window moved by the user may still carry the previous GUID.
        DWORD cloak = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloak, sizeof(cloak))) &&
            (cloak & DWM_CLOAKED_SHELL)) {
            GUID actual{};
            if (!GetWindowDesktopGuid(h, actual, true) || !IsEqualGUID(actual, currentDesktop))
                continue;
        }

        if (ShowWindowAndShowTaskbarIcon(h, true)) g_snapUnhiddenWindows.push_back(h);
        else HideWindowAndHideTaskbarIcon(h, true);
    }
    g_InSnapWindows = !g_snapUnhiddenWindows.empty();
    if (!g_InSnapWindows) g_altTabPreviewDesktop = GUID{};
    if (g_InSnapWindows) {
        // Let the restored windows paint before the native switcher captures them.
        for (HWND h : g_snapUnhiddenWindows)
            RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        DwmFlush();
    }
    return g_InSnapWindows;
}

static void FinishAltTabPreview(HWND winner, ReadCurrentDesktop readCurrentDesktop) {
    if (!g_InSnapWindows) return;

    std::wstring selectedProcess;
    GUID currentDesktop{};
    if (IsWindow(winner) && readCurrentDesktop(currentDesktop) &&
        IsEqualGUID(currentDesktop, g_altTabPreviewDesktop)) {
        const HWND root = GetAncestor(winner, GA_ROOT);
        const HWND owner = GetAncestor(winner, GA_ROOTOWNER);
        HWND selected = root;
        if (std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), selected) ==
            g_snapUnhiddenWindows.end()) selected = owner;
        if (std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), selected) !=
            g_snapUnhiddenWindows.end()) {
            if (TaggedDesktopMatches(selected, currentDesktop))
                selectedProcess = ReadTaggedExe(selected);
        } else if (root) {
            // A visible window can be selected while its siblings are being previewed.
            selectedProcess = GetRealProcessName(root);
        }
    }

    const bool keepProcess = IsIdentifiedProcess(selectedProcess);
    for (HWND h : g_snapUnhiddenWindows) {
        if (!g_sealedWindowStates.contains(h)) continue;
        if (keepProcess && TaggedDesktopMatches(h, currentDesktop) &&
            _wcsicmp(ReadTaggedExe(h).c_str(), selectedProcess.c_str()) == 0 &&
            ShowWindowAndShowTaskbarIcon(h)) continue;
        HideWindowAndHideTaskbarIcon(h, true);
    }
    g_snapUnhiddenWindows.clear();
    g_altTabPreviewDesktop = GUID{};
    g_InSnapWindows = false;
}

void ExecuteCollectRestore() {
    CancelAltTabCollectTimer(g_hLassoCoordinator);
    FinishAltTabPreview(nullptr);
}

static void ScheduleAltTabCollect(HWND coordinator) {
    if (!g_InSnapWindows) return;
    const auto now = GetTickCount64();
    g_altTabNextCollectAt = now + 150;
    g_altTabCollectDeadline = now + 3000;
    if (!SetTimer(coordinator, TIMER_ID_COLLECT, 150, nullptr)) ExecuteCollectRestore();
}

static bool BeginAltTabPreview(HWND coordinator, HWND foreground,
                               ReadCurrentDesktop readCurrentDesktop = nullptr) {
    if (g_altTabNextCollectAt) {
        // A new Alt chord arrived before the previous release was collected.
        CancelAltTabCollectTimer(coordinator);
        FinishAltTabPreview(IsSwitcherWindow(foreground) ? nullptr : foreground,
            readCurrentDesktop ? readCurrentDesktop : GetCurrentDesktopGuid);
    }
    return TryUnHideWindowsForAltTab(readCurrentDesktop, foreground);
}

// ============================================================================
// 3. 核心钩子回调函数
// ============================================================================

// The coordinator owns window state and performs cross-process icon hit testing.
#define WM_LASSO_HIDE_CURRENT (WM_APP + 11)
#define WM_LASSO_HIDE_OTHERS  (WM_APP + 12)
#define WM_LASSO_ALT_TAB      (WM_APP + 13)
#define WM_LASSO_ALT_UP       (WM_APP + 14)
#define WM_LASSO_ESCAPE       (WM_APP + 16)
#define WM_LASSO_TOGGLE_PRIMARY (WM_APP + 17)
#define WM_LASSO_RESTORE_PRIMARY (WM_APP + 18)
#define WM_LASSO_PRIMARY_LEFT_CLICK (WM_APP + 19)
#define WM_LASSO_PRIMARY_RIGHT_CLICK (WM_APP + 20)

LRESULT CALLBACK LowLevelKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION)
        return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
    const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
    const bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    const bool up = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    HWND coordinator = g_hLassoCoordinator;
    if (!coordinator) return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
    if (down && !(key->flags & LLKHF_INJECTED))
        PostMessageW(coordinator, WM_LASSO_RESTORE_PRIMARY, 0, 0);

    // Injected Tab is allowed through so the native Alt+Tab UI handles it.
    if (key->vkCode == VK_TAB && (key->flags & LLKHF_ALTDOWN) && down) {
        if (key->flags & LLKHF_INJECTED)
            return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
        const bool reverse = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const HWND foreground = GetForegroundWindow();
        if (PostMessageW(coordinator, WM_LASSO_ALT_TAB, reverse,
                reinterpret_cast<LPARAM>(foreground))) return 1;
    }
    if (key->vkCode == VK_ESCAPE && down)
        PostMessageW(coordinator, WM_LASSO_ESCAPE, 0, 0);

    if (up && !(key->flags & LLKHF_INJECTED) &&
        (key->vkCode == VK_MENU || key->vkCode == VK_LMENU || key->vkCode == VK_RMENU)) {
        const bool rightAlt = key->vkCode == VK_RMENU ||
            (key->vkCode == VK_MENU && (key->flags & LLKHF_EXTENDED));
        const int otherAlt = rightAlt ? VK_LMENU : VK_RMENU;
        if (!(GetAsyncKeyState(otherAlt) & 0x8000))
            PostMessageW(coordinator, WM_LASSO_ALT_UP, 0, 0);
    }

    if (down && key->vkCode == 'C' && !(key->flags & LLKHF_INJECTED)) {
        bool win = (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
                   (GetAsyncKeyState(VK_RWIN) & 0x8000);
        if (win) {
            const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            HWND foreground = GetForegroundWindow();
            UINT message = shift ? WM_LASSO_HIDE_OTHERS : WM_LASSO_HIDE_CURRENT;
            if (PostMessageW(coordinator, message,
                    reinterpret_cast<WPARAM>(foreground), 0)) {
                SuppressWinKeyMenuModern();
                return 1;
            }
        }
    }
    return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

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

// The native Show Desktop button is a real child HWND on the primary taskbar.
// Fail closed when Explorer does not expose it; never guess a broad right-edge band.
static constexpr wchar_t SHOW_DESKTOP_BUTTON_CLASS[] = L"TrayShowDesktopButtonWClass";
static bool g_showDesktopClickCaptured = false;

static HWND FindShowDesktopButton(HWND primaryTaskbar) {
    if (!IsWindow(primaryTaskbar)) return nullptr;
    struct SearchContext { HWND button = nullptr; } context;
    EnumChildWindows(primaryTaskbar, [](HWND child, LPARAM parameter) -> BOOL {
        auto* search = reinterpret_cast<SearchContext*>(parameter);
        if (!IsWindowVisible(child)) return TRUE;
        wchar_t className[96]{};
        if (GetClassNameW(child, className, static_cast<int>(std::size(className))) &&
            wcscmp(className, SHOW_DESKTOP_BUTTON_CLASS) == 0) {
            search->button = child;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
    return context.button;
}

static bool IsPointInsideShowDesktopButton(HWND primaryTaskbar, POINT point) {
    const HWND button = FindShowDesktopButton(primaryTaskbar);
    RECT bounds{};
    return button && GetWindowRect(button, &bounds) && PtInRect(&bounds, point);
}

struct PrimaryWindowTransparencyState {
    LONG_PTR exStyle = 0;
    BYTE alpha = 255;
    COLORREF colorKey = 0;
    DWORD layerFlags = 0;
    bool hasLayerAttributes = false;
};

static std::unordered_map<HWND, PrimaryWindowTransparencyState> g_transparentPrimaryWindows;

struct DesktopIconClick {
    UINT button = 0;
    POINT point{};
    DWORD time = 0;
};

static DesktopIconClick g_lastDesktopIconClick;

static bool SetWindowExtendedStyle(HWND h, LONG_PTR style) {
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR previous = SetWindowLongPtrW(h, GWL_EXSTYLE, style);
    return previous != 0 || GetLastError() == ERROR_SUCCESS;
}

static bool ApplyPrimaryWindowTransparency(HWND h) {
    if (!IsWindow(h)) return false;
    if (g_transparentPrimaryWindows.contains(h)) return true;

    PrimaryWindowTransparencyState original;
    original.exStyle = GetWindowLongPtrW(h, GWL_EXSTYLE);
    if (original.exStyle & WS_EX_LAYERED) {
        // Do not overwrite layered windows whose original attributes cannot
        // be queried (for example, windows using UpdateLayeredWindow).
        if (!GetLayeredWindowAttributes(h, &original.colorKey,
                &original.alpha, &original.layerFlags)) return false;
        original.hasLayerAttributes = true;
    }

    const LONG_PTR transparentStyle = original.exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT;
    if (!SetWindowExtendedStyle(h, transparentStyle)) return false;

    if (!SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA)) {
        if (!(original.exStyle & WS_EX_LAYERED))
            SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA);
        SetWindowExtendedStyle(h, original.exStyle);
        SetWindowPos(h, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
            SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS);
        return false;
    }

    if (!SetWindowPos(h, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
            SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS)) {
        if (original.exStyle & WS_EX_LAYERED)
            SetLayeredWindowAttributes(h, original.colorKey, original.alpha, original.layerFlags);
        else
            SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA);
        SetWindowExtendedStyle(h, original.exStyle);
        return false;
    }

    // Preserve taskbar identity: never add/remove WS_EX_APPWINDOW or WS_EX_TOOLWINDOW.
    g_transparentPrimaryWindows.emplace(h, original);
    return true;
}

static bool RestorePrimaryWindowTransparency(HWND h) {
    const auto it = g_transparentPrimaryWindows.find(h);
    if (it == g_transparentPrimaryWindows.end()) return true;
    if (!IsWindow(h)) {
        g_transparentPrimaryWindows.erase(it);
        return true;
    }

    const auto original = it->second;
    if (!(original.exStyle & WS_EX_LAYERED)) {
        if (!SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA)) return false;
        if (!SetWindowExtendedStyle(h, original.exStyle)) {
            SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
            return false;
        }
    } else {
        if (!SetWindowExtendedStyle(h, original.exStyle)) return false;
        if (original.hasLayerAttributes &&
            !SetLayeredWindowAttributes(h, original.colorKey, original.alpha, original.layerFlags)) {
            SetWindowExtendedStyle(h, original.exStyle | WS_EX_TRANSPARENT);
            SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
            return false;
        }
    }

    SetWindowPos(h, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
        SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS);
    g_transparentPrimaryWindows.erase(it);
    return true;
}

static void RestoreAllPrimaryWindowTransparency() {
    g_lastDesktopIconClick = {};
    for (auto it = g_transparentPrimaryWindows.begin();
         it != g_transparentPrimaryWindows.end();) {
        const HWND h = it->first;
        ++it;
        RestorePrimaryWindowTransparency(h);
    }
}

static bool IsPrimaryDesktopIconAt(POINT point) {
    if (MonitorFromPoint(point, MONITOR_DEFAULTTONULL) !=
        MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY)) return false;

    winrt::com_ptr<IAccessible> item;
    VARIANT child{}, role{};
    HWND list = nullptr;
    const bool isItem = SUCCEEDED(AccessibleObjectFromPoint(point, item.put(), &child)) && item &&
        SUCCEEDED(item->get_accRole(child, &role)) && role.vt == VT_I4 &&
        role.lVal == ROLE_SYSTEM_LISTITEM &&
        SUCCEEDED(WindowFromAccessibleObject(item.get(), &list));
    VariantClear(&role);
    VariantClear(&child);
    if (!isItem || !list) return false;

    wchar_t listClass[32]{}, parentClass[32]{}, rootClass[32]{};
    return GetClassNameW(list, listClass, static_cast<int>(std::size(listClass))) &&
        wcscmp(listClass, L"SysListView32") == 0 &&
        GetClassNameW(GetParent(list), parentClass, static_cast<int>(std::size(parentClass))) &&
        wcscmp(parentClass, L"SHELLDLL_DefView") == 0 &&
        GetClassNameW(GetAncestor(list, GA_ROOT), rootClass, static_cast<int>(std::size(rootClass))) &&
        (wcscmp(rootClass, L"Progman") == 0 || wcscmp(rootClass, L"WorkerW") == 0);
}

static RECT MouseGestureBounds(POINT point, int widthMetric, int heightMetric) {
    const int width = GetSystemMetrics(widthMetric);
    const int height = GetSystemMetrics(heightMetric);
    const LONG left = point.x - width / 2;
    const LONG top = point.y - height / 2;
    return RECT{left, top, left + width, top + height};
}

static void HandlePrimaryDesktopClick(UINT button, POINT point, DWORD time,
    bool (*isDesktopIconAt)(POINT) = IsPrimaryDesktopIconAt) {
    if (g_transparentPrimaryWindows.empty()) return;
    const RECT doubleClickBounds = MouseGestureBounds(g_lastDesktopIconClick.point,
        SM_CXDOUBLECLK, SM_CYDOUBLECLK);
    // A right-click is always an allowed desktop-icon operation. Only a
    // left double-click opens the icon and therefore restores the windows.
    const bool doubleClick = button == WM_LASSO_PRIMARY_LEFT_CLICK &&
        g_lastDesktopIconClick.button == WM_LASSO_PRIMARY_LEFT_CLICK &&
        time - g_lastDesktopIconClick.time <= GetDoubleClickTime() &&
        PtInRect(&doubleClickBounds, point);
    if (doubleClick || !isDesktopIconAt(point)) {
        RestoreAllPrimaryWindowTransparency();
        return;
    }
    g_lastDesktopIconClick = {button, point, time};
}

// Keep the original candidate filters: current desktop, visible, not minimized,
// no MTU_EXE_TAG, no owner, and strictly on the primary monitor.
struct ToggleContext {
    std::vector<HWND>* pVector;
    HMONITOR hPrimaryMonitor;
};

BOOL CALLBACK EnumWindowToggleProc(HWND hWnd, LPARAM lParam) {
    auto* pCtx = reinterpret_cast<ToggleContext*>(lParam);
    if (!pCtx || !pCtx->pVector || !IsValidAppWindow(hWnd)) return TRUE;
    if (!IsWindowOnCurrentVirtualDesktop(hWnd)) return TRUE;
    if (IsIconic(hWnd) || !IsWindowVisible(hWnd)) return TRUE;
    if (GetPropW(hWnd, L"MTU_EXE_TAG") != nullptr) return TRUE;
    if (GetWindow(hWnd, GW_OWNER) != nullptr) return TRUE;

    RECT winRect{};
    if (FAILED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS,
            &winRect, sizeof(winRect))) && !GetPhysicalRect(hWnd, &winRect))
        return TRUE;
    const HMONITOR hWinMonitor = MonitorFromRect(&winRect, MONITOR_DEFAULTTONULL);
    if (!hWinMonitor || hWinMonitor != pCtx->hPrimaryMonitor) return TRUE;
    pCtx->pVector->push_back(hWnd);
    return TRUE;
}

static void TogglePrimaryWindows() {
    if (!g_transparentPrimaryWindows.empty()) {
        RestoreAllPrimaryWindowTransparency();
        return;
    }

    POINT origin{0, 0};
    const HMONITOR primary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    std::vector<HWND> candidates;
    ToggleContext context{&candidates, primary};
    EnumWindows(EnumWindowToggleProc, reinterpret_cast<LPARAM>(&context));
    for (HWND h : candidates)
        ApplyPrimaryWindowTransparency(h);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // Only the hook thread reads the drag state. Ordinary movement posts nothing.
    static WPARAM pressedButton = 0;
    static RECT dragBounds{};
    if (nCode == HC_ACTION && wParam == WM_LBUTTONUP && g_showDesktopClickCaptured) {
        g_showDesktopClickCaptured = false;
        return 1;
    }

    if (nCode == HC_ACTION && g_hLassoCoordinator) {
        const HWND coordinator = g_hLassoCoordinator;
        const auto* mouse = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        switch (wParam) {
        case WM_LBUTTONDOWN: {
            const HWND primaryTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
            if (IsPointInsideShowDesktopButton(primaryTaskbar, mouse->pt) &&
                PostMessageW(coordinator, WM_LASSO_TOGGLE_PRIMARY, 0, 0)) {
                g_showDesktopClickCaptured = true;
                pressedButton = 0;
                return 1;
            }
            [[fallthrough]];
        }
        case WM_RBUTTONDOWN:
            pressedButton = wParam;
            dragBounds = MouseGestureBounds(mouse->pt, SM_CXDRAG, SM_CYDRAG);
            PostMessageW(coordinator, wParam == WM_LBUTTONDOWN ? WM_LASSO_PRIMARY_LEFT_CLICK :
                WM_LASSO_PRIMARY_RIGHT_CLICK, mouse->pt.x, mouse->pt.y);
            break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if ((pressedButton == WM_LBUTTONDOWN && wParam == WM_LBUTTONUP) ||
                (pressedButton == WM_RBUTTONDOWN && wParam == WM_RBUTTONUP)) pressedButton = 0;
            break;
        case WM_MOUSEMOVE:
            if (!pressedButton || PtInRect(&dragBounds, mouse->pt)) break;
            [[fallthrough]];
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            pressedButton = 0;
            PostMessageW(coordinator, WM_LASSO_RESTORE_PRIMARY, 0, 0);
            break;
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

static HWND g_taskViewWindow = nullptr;

void CALLBACK WinEventProc(HWINEVENTHOOK h, DWORD event, HWND hwnd, LONG idO, LONG idC, DWORD dwT, DWORD dwM) {
	// 基础过滤：只处理窗口对象
	if (hwnd == NULL || idO != OBJID_WINDOW) return;
    if (event == EVENT_OBJECT_DESTROY && idC == CHILDID_SELF) {
        if (g_taskViewWindow == hwnd) g_taskViewWindow = nullptr;
        // A HWND can be recycled after destruction; never restore stale state
        // onto a different window that later receives the same handle value.
        g_sealedWindowStates.erase(hwnd);
        g_restoredWindowShows.erase(hwnd);
        auto forget = [hwnd](std::vector<HWND>& v) {
            v.erase(std::remove(v.begin(), v.end(), hwnd), v.end());
        };
        forget(g_HiddenWindowsCache);
        forget(g_snapUnhiddenWindows);
        g_transparentPrimaryWindows.erase(hwnd);
        return;
    }

	// 捕获插拔显示器、分辨率切换、桌面重建事件
	if (event == EVENT_SYSTEM_DESKTOPSWITCH) {
		UpdateSystemDpiScalingStatus(); // 刷新屏幕缩放标志位
		return;
	}

	// --- 1. 获取基础信息 ---
	wchar_t cls[256] = { 0 };
	wchar_t title[256] = { 0 };
	GetClassNameW(hwnd, cls, 256);
	GetWindowTextW(hwnd, title, 256);
	std::wstring className = cls;
	std::wstring windowTitle = title;

    bool restoredByLasso = false;
    if (event == EVENT_OBJECT_SHOW && idC == CHILDID_SELF) {
        const auto show = g_restoredWindowShows.find(hwnd);
        if (show != g_restoredWindowShows.end()) {
            // WinEvent's timestamp and the low 32 bits of uptime use the same
            // clock. Unsigned subtraction also handles the timestamp wrap.
            restoredByLasso = dwM - static_cast<DWORD>(show->second) <= 3000;
            if (restoredByLasso || GetTickCount64() - show->second > 3000)
                g_restoredWindowShows.erase(show);
        }
    }

    // A new window can wake its local app group. A show event generated by
    // Alt+Tab must not unseal the other windows in that same group.
    if (!restoredByLasso && (event == EVENT_OBJECT_SHOW || event == EVENT_OBJECT_CREATE) &&
        idC == CHILDID_SELF && !g_InSnapWindows &&
        !IsSwitcherWindow(hwnd) &&
        IsWindowOnCurrentVirtualDesktop(hwnd)) {

        // A show/create event for an already tagged window must not trigger
        // a recursive reactivation as we change its styles.
        const bool tagged = GetPropW(hwnd, L"MTU_EXE_TAG") != nullptr;
        if (!tagged) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid && pid != GetCurrentProcessId()) {
                std::wstring process = GetRealProcessName(hwnd);
                if (_wcsicmp(process.c_str(), L"explorer.exe") == 0 &&
                    className != L"CabinetWClass") process.clear();
                if (!process.empty() && process != L"Unknown" &&
                    _wcsicmp(process.c_str(), L"ApplicationFrameHost.exe") != 0 &&
                    !g_sealedWindowStates.empty()) {
                    RefreshWindowCache();
                    g_HiddenWindowsCache.erase(
                        std::remove_if(g_HiddenWindowsCache.begin(),
                            g_HiddenWindowsCache.end(),
                            [&](HWND h) {
                                if (!IsWindow(h)) return true;
                                if (!CanUnsealOnCurrentDesktop(h)) return false;
                                HANDLE prop = GetPropW(h, L"MTU_EXE_TAG");
                                wchar_t taggedName[MAX_PATH]{};
                                if (!prop || !GlobalGetAtomNameW(
                                    static_cast<ATOM>(reinterpret_cast<UINT_PTR>(prop)),
                                    taggedName, MAX_PATH)) return false;
                                if (_wcsicmp(process.c_str(), taggedName) != 0) return false;
                                return ShowWindowAndShowTaskbarIcon(h);
                            }), g_HiddenWindowsCache.end());
                }
            }
        }
    }

	// --- 2. 最小化/恢复监控 (仅针对窗口自身 idC == CHILDID_SELF) ---
	if (idC == CHILDID_SELF) {
        // A Shell minimize event from another desktop must not wake or regroup it.
        if ((event == EVENT_SYSTEM_MINIMIZESTART || event == EVENT_SYSTEM_MINIMIZEEND) &&
            !IsWindowOnCurrentVirtualDesktop(hwnd)) return;
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
    // Win+Tab's notification is independent of the Alt+Tab preview session.
    const bool isTaskViewWindow = className == L"XamlExplorerHostIslandWindow" &&
        windowTitle == L"任务视图";
    if (isTaskViewWindow && idC == CHILDID_SELF &&
        (event == EVENT_OBJECT_SHOW || event == EVENT_SYSTEM_FOREGROUND)) {
        RECT rc{};
        if (g_taskViewWindow != hwnd && GetPhysicalRect(hwnd, &rc) && rc.right - rc.left >= 500) {
            g_taskViewWindow = hwnd;
            ShowWin11NativeNotification(L"待完成", L"Win+Tab可完成窗口转移");
        }
    } else if (event == EVENT_SYSTEM_FOREGROUND) {
        g_taskViewWindow = nullptr;
    }
}

// A message-only window on a separate worker thread serializes COM, WinEvent,
// taskbar and hide/restore work. Hook callbacks live on the main message thread.
static void HideCurrentApplication(HWND foreground, ReadWindowDesktop readDesktop = GetWindowDesktopGuid,
                                   bool (*isOnCurrentDesktop)(HWND) = IsWindowOnCurrentVirtualDesktop) {
    RestoreAllPrimaryWindowTransparency();
    if (!IsWindow(foreground)) return;
    HWND root = GetAncestor(foreground, GA_ROOT);
    if (!root || !isOnCurrentDesktop(root)) return;
    wchar_t className[256]{};
    if (!GetClassNameW(root, className, 256)) return;
    // Never seal the Desktop, Shell/taskbar, or task-view UI.
    if (wcscmp(className, L"Progman") == 0 ||
        wcscmp(className, L"Shell_TrayWnd") == 0 ||
        wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
        IsSwitcherWindow(root)) return;

    HideCurrentFilter filter{GetRealProcessName(root), readDesktop};
    if (!IsIdentifiedProcess(filter.process)) {
        // A shared host cannot identify sibling windows from the same app.
        RequestHostedUwpClose(root);
        return;
    }
    EnumWindows(EnumHideWindowsProc, reinterpret_cast<LPARAM>(&filter));
}

static void HideOtherApplications(HWND foreground) {
    RestoreAllPrimaryWindowTransparency();
    if (!IsWindow(foreground)) return;
    HWND root = GetAncestor(foreground, GA_ROOT);
    if (!root || !IsWindowOnCurrentVirtualDesktop(root)) return;
    wchar_t className[256]{};
    if (!GetClassNameW(root, className, 256)) return;
    if (wcscmp(className, L"Progman") == 0 ||
        wcscmp(className, L"Shell_TrayWnd") == 0 ||
        wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
        IsSwitcherWindow(root)) return;
    CloseFilter filter;
    filter.currentAppProcName = GetRealProcessName(root);
    if (filter.currentAppProcName != L"Unknown" &&
        !filter.currentAppProcName.empty())
        EnumWindows(EnumHideOthersProc, reinterpret_cast<LPARAM>(&filter));
}

static LRESULT CALLBACK CoordinatorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_LASSO_HIDE_CURRENT:
        HideCurrentApplication(reinterpret_cast<HWND>(wp));
        return 0;
    case WM_LASSO_HIDE_OTHERS:
        HideOtherApplications(reinterpret_cast<HWND>(wp));
        return 0;
    case WM_LASSO_ALT_TAB: {
        const HWND foregroundAtKeyDown = reinterpret_cast<HWND>(lp);
        // Resolve against the live foreground on the coordinator thread; the
        // HWND captured by the low-level hook can be stale after a desktop switch.
        const HWND foregroundNow = GetForegroundWindow();
        const HWND previewAnchor = IsWindow(foregroundNow) ? foregroundNow : foregroundAtKeyDown;
        BeginAltTabPreview(hwnd, previewAnchor);
        INPUT input[6]{};
        const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        const bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const auto count = static_cast<UINT>(BuildAltTabInput(input, altDown, shiftDown, wp != 0));
        if (SendInput(count, input, sizeof(INPUT)) != count) ExecuteCollectRestore();
        else if (!altDown) ScheduleAltTabCollect(hwnd);
        else CancelAltTabCollectTimer(hwnd);
        return 0;
    }
    case WM_LASSO_ALT_UP:
        ScheduleAltTabCollect(hwnd);
        return 0;
    case WM_LASSO_ESCAPE:
        if (g_InSnapWindows) ExecuteCollectRestore();
        return 0;
    case WM_LASSO_TOGGLE_PRIMARY:
        TogglePrimaryWindows();
        return 0;
    case WM_LASSO_RESTORE_PRIMARY:
        RestoreAllPrimaryWindowTransparency();
        return 0;
    case WM_LASSO_PRIMARY_LEFT_CLICK:
    case WM_LASSO_PRIMARY_RIGHT_CLICK:
        HandlePrimaryDesktopClick(msg, POINT{static_cast<LONG>(wp), static_cast<LONG>(lp)},
            static_cast<DWORD>(GetMessageTime()));
        return 0;
    case WM_TIMER:
        if (wp == TIMER_ID_COLLECT) {
            OnCollectTimer(hwnd, wp);
            return 0;
        }
        break;
    case WM_CLOSE:
        RestoreAllSealedWindows();
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI CoordinatorThreadProc(void* readyPtr) {
    HANDLE ready = static_cast<HANDLE>(readyPtr);
    try { winrt::init_apartment(winrt::apartment_type::single_threaded); }
    catch (...) { SetEvent(ready); return 1; }

    WNDCLASSW wc{};
    wc.lpfnWndProc = CoordinatorWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"LassoWindowsCoordinator";
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        SetEvent(ready);
        winrt::uninit_apartment();
        return 1;
    }
    g_hLassoCoordinator = CreateWindowExW(0, wc.lpszClassName, L"",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!g_hLassoCoordinator) {
        SetEvent(ready);
        winrt::uninit_apartment();
        return 1;
    }
    UpdateSystemDpiScalingStatus();
    RegisterAppShortcut();
    g_hSysHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_MINIMIZEEND,
        nullptr, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    g_hObjHook = SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_SHOW,
        nullptr, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    HWINEVENTHOOK desktopHook = SetWinEventHook(EVENT_SYSTEM_DESKTOPSWITCH,
        EVENT_SYSTEM_DESKTOPSWITCH, nullptr, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    SetEvent(ready);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (g_hSysHook) UnhookWinEvent(g_hSysHook);
    if (g_hObjHook) UnhookWinEvent(g_hObjHook);
    if (desktopHook) UnhookWinEvent(desktopHook);
    RestoreAllSealedWindows();
    g_hLassoCoordinator = nullptr;
    winrt::uninit_apartment();
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Global\\MyWindowManagerUniqueMutex_12345");
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        return 0;
    }
    // Initialize DPI before any UI-dependent work.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE worker = ready ? CreateThread(nullptr, 0, CoordinatorThreadProc, ready, 0, nullptr) : nullptr;
    if (!worker || WaitForSingleObject(ready, 15000) != WAIT_OBJECT_0 || !g_hLassoCoordinator) {
        if (worker) {
            WaitForSingleObject(worker, INFINITE);
            CloseHandle(worker);
        }
        if (ready) CloseHandle(ready);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }
    CloseHandle(ready);

    g_hKeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyProc, hInst, 0);
    g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInst, 0);
    if (!g_hKeyHook || !g_hMouseHook) {
        if (g_hKeyHook) UnhookWindowsHookEx(g_hKeyHook);
        if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
        PostMessageW(g_hLassoCoordinator, WM_CLOSE, 0, 0);
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hKeyHook);
    UnhookWindowsHookEx(g_hMouseHook);
    if (g_hLassoCoordinator) PostMessageW(g_hLassoCoordinator, WM_CLOSE, 0, 0);
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}
