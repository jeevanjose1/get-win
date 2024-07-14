#include <node.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <tuple>

#ifdef _WIN32
#include <Windows.h>

void getOpenWindowsInfoWindows(std::vector<std::tuple<std::string, bool, std::string>>& windows) {
    windows.clear();

    HWND hwnd = GetTopWindow(nullptr);  // Get handle to the top-level window
    std::unordered_set<HWND> seenWindows;

    while (hwnd != nullptr) {
        if (IsWindowVisible(hwnd)) {
            if (seenWindows.find(hwnd) == seenWindows.end()) {
                seenWindows.insert(hwnd);

                char titleBuffer[256];
                GetWindowTextA(hwnd, titleBuffer, sizeof(titleBuffer));  // Get window title

                // Check if window title is not empty
                if (strlen(titleBuffer) > 0) {
                    // Check if window is active (focused)
                    bool isActive = (GetForegroundWindow() == hwnd);

                    // Extract application name from window title
                    std::string title = titleBuffer;
                    size_t pos = title.find(" - ");
                    if (pos != std::string::npos) {
                        std::string applicationName = title.substr(0, pos);
                        windows.emplace_back(std::make_tuple(titleBuffer, isActive, applicationName));
                    } else {
                        // If separator not found, use the whole title as application name
                        windows.emplace_back(std::make_tuple(titleBuffer, isActive, title));
                    }
                }
            }
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);  // Get handle to the next window
    }
}

#elif __APPLE__
#include <Cocoa/Cocoa.h>

void getOpenWindowsInfoMac(std::vector<std::tuple<std::string, bool, std::string>>& windows) {
    windows.clear();

    NSArray *windowsList = [NSRunningApplication runningApplicationsWithBundleIdentifier:@""];

    for (NSRunningApplication *app in windowsList) {
        NSString *title = app.localizedName;

        // Check if window title is not empty
        if ([title length] > 0) {
            bool isActive = [app isActive];
            std::string applicationName = [title UTF8String];

            windows.emplace_back(std::make_tuple("", isActive, applicationName));
        }
    }
}

#else
#include <X11/Xlib.h>

void getOpenWindowsInfoLinux(std::vector<std::tuple<std::string, bool, std::string>>& windows) {
    windows.clear();

    Display* display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);
    Window parent, *children;
    unsigned int numChildren;

    XQueryTree(display, root, &root, &parent, &children, &numChildren);
    std::unordered_set<Window> seenWindows;

    for (unsigned int i = 0; i < numChildren; ++i) {
        if (seenWindows.find(children[i]) == seenWindows.end()) {
            seenWindows.insert(children[i]);

            char* windowTitle = nullptr;
            if (XFetchName(display, children[i], &windowTitle)) {
                // Check if window title is not empty
                if (strlen(windowTitle) > 0) {
                    // Check if window is active (focused)
                    bool isActive = (children[i] == XGetInputFocus(display, &root, nullptr));

                    // Extract application name from window title
                    std::string title = windowTitle;
                    size_t pos = title.find(" - ");
                    if (pos != std::string::npos) {
                        std::string applicationName = title.substr(0, pos);
                        windows.emplace_back(std::make_tuple(windowTitle, isActive, applicationName));
                    } else {
                        // If separator not found, use the whole title as application name
                        windows.emplace_back(std::make_tuple(windowTitle, isActive, title));
                    }
                }
                XFree(windowTitle);
            }
        }
    }

    XFree(children);
    XCloseDisplay(display);
}
#endif

// Function to convert vector of tuples to v8::Array
v8::Local<v8::Array> ConvertWindowsToV8Array(const std::vector<std::tuple<std::string, bool, std::string>>& windows) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Array> result = v8::Array::New(isolate, windows.size());

    for (size_t i = 0; i < windows.size(); ++i) {
        v8::Local<v8::Object> obj = v8::Object::New(isolate);
        obj->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "title").ToLocalChecked(), v8::String::NewFromUtf8(isolate, std::get<0>(windows[i]).c_str()).ToLocalChecked());
        obj->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "isActive").ToLocalChecked(), v8::Boolean::New(isolate, std::get<1>(windows[i])));
        
        std::string tabName = std::get<2>(windows[i]);
        size_t pos = tabName.find(" - ");
        if (pos != std::string::npos) {
            tabName = tabName.substr(pos + 3);
        } else {
            pos = tabName.find(" | ");
            if (pos != std::string::npos) {
                tabName = tabName.substr(pos + 3);
            }
        }
        obj->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "tabName").ToLocalChecked(), v8::String::NewFromUtf8(isolate, tabName.c_str()).ToLocalChecked());

        result->Set(isolate->GetCurrentContext(), i, obj);
    }

    return result;
}

// Function to get window information based on platform
void GetOpenWindowsInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::vector<std::tuple<std::string, bool, std::string>> windows;

#ifdef _WIN32
    getOpenWindowsInfoWindows(windows);
#elif __APPLE__
    getOpenWindowsInfoMac(windows);
#else
    getOpenWindowsInfoLinux(windows);
#endif

    v8::Local<v8::Array> result = ConvertWindowsToV8Array(windows);
    args.GetReturnValue().Set(result);
}

// Register the addon with Node.js
void Init(v8::Local<v8::Object> exports) {
    NODE_SET_METHOD(exports, "getOpenWindowsInfo", GetOpenWindowsInfo);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
