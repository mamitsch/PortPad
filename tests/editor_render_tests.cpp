#include "AppState.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace portpad;
int main() {
    char directory[]="/tmp/portpad-render-XXXXXX";
    if(!mkdtemp(directory)) return 1;
    const auto path=std::filesystem::path(directory)/"development.ini";
    { std::ofstream f(path); f<<"# PortPad development preview\n[editor]\nfont = monospace\nbackup = enabled\n\n# UTF-8: café\n"; for(int i=0;i<25;++i) f<<"line_"<<i<<" = "<<std::string(80,'x')<<'\n'; }
    int result=0;
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0 || TTF_Init()!=0) return 1;
    try {
        AppState app; Input input(PORTPAD_TEST_KEYMAP); Renderer renderer(PORTPAD_TEST_FONT);
        if(!app.openEditor(path)) throw std::runtime_error("open preview");
        renderer.draw(app,input);
        app.handle(InputAction::PageDown); renderer.draw(app,input);
        for(int i=0;i<70;++i) app.handle(InputAction::Right);
        renderer.draw(app,input);
        app.editorCommand(EditorCommand::Find); app.insertText("font"); renderer.draw(app,input); app.editorCommand(EditorCommand::Newline); renderer.draw(app,input);
        app.editorCommand(EditorCommand::GoToLine); app.insertText("1"); app.editorCommand(EditorCommand::Newline);
        app.handle(InputAction::Menu); renderer.draw(app,input);
        for(int i=0;i<8;++i) app.handle(InputAction::Down);
        app.handle(InputAction::Confirm); renderer.draw(app,input);
        app.handle(InputAction::Up); app.handle(InputAction::Confirm);
        app.handle(InputAction::Confirm); app.insertText("# Editable buffer\n"); renderer.draw(app,input);
        // Optional inspectable screenshot, kept out of normal test output.
        if(const char* output=SDL_getenv("PORTPAD_EDITOR_SCREENSHOT")) {
            auto* window=SDL_GetWindowFromID(1); auto* render=SDL_GetRenderer(window);
            auto* pixels=SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
            if(!pixels || SDL_RenderReadPixels(render,nullptr,pixels->format->format,pixels->pixels,pixels->pitch)!=0 || SDL_SaveBMP(pixels,output)!=0) throw std::runtime_error(SDL_GetError());
            SDL_FreeSurface(pixels);
        }
        app.handle(InputAction::PageDown); renderer.draw(app,input);
        app.handle(InputAction::Menu); app.editorCommand(EditorCommand::SaveAs); renderer.draw(app,input);
        app.editorCommand(EditorCommand::Newline); renderer.draw(app,input);
        app.handle(InputAction::Back); app.requestQuit(); renderer.draw(app,input);
        app.handle(InputAction::Confirm); // Cancel close, retain edits.
        if(!app.running() || !app.editor().buffer().dirty()) throw std::runtime_error("quit cancel");
        app.requestQuit(); app.handle(InputAction::Up); app.handle(InputAction::Confirm);
        if(app.running()) throw std::runtime_error("quit discard");
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; result=1; }
    TTF_Quit(); SDL_Quit(); std::filesystem::remove_all(directory); return result;
}
