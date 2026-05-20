with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/WebManager.cpp", "r", encoding="utf-8") as f:
    code = f.read()

# Add checkAuth
check_auth_code = """
bool WebManager::checkAuth() {
    if (!_server.authenticate("Alex", "AlexPASS")) {
        _server.requestAuthentication();
        return false;
    }
    return true;
}

void WebManager::handleRoot() {
"""

code = code.replace("void WebManager::handleRoot() {", check_auth_code)

# Add checkAuth() calls
code = code.replace("void WebManager::handleRoot() {\n", "void WebManager::handleRoot() {\n    if (!checkAuth()) return;\n")
code = code.replace("void WebManager::handleSave() {\n", "void WebManager::handleSave() {\n    if (!checkAuth()) return;\n")
code = code.replace("void WebManager::handlePager() {\n", "void WebManager::handlePager() {\n    if (!checkAuth()) return;\n")

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/WebManager.cpp", "w", encoding="utf-8") as f:
    f.write(code)
