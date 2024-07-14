const getActiveWindow = require("./build/Release/getwindowsinfo");
const activeWindows = getActiveWindow.getOpenWindowsInfo();
module.exports = activeWindows;
