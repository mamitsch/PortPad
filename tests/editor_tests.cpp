#include "AppState.hpp"
#include "DocumentIO.hpp"
#include "EditorState.hpp"
#include "TextBuffer.hpp"
#include "VirtualKeyboard.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
using namespace portpad;
namespace fs = std::filesystem;
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read(const fs::path& p) { std::ifstream f(p, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}}; }
void write(const fs::path& p, const std::string& s) { std::ofstream f(p, std::ios::binary); f << s; check(bool(f), "fixture write"); }
struct Fixture {
    fs::path path, cwd = fs::current_path();
    Fixture() { char name[] = "/tmp/portpad-editor-XXXXXX"; auto p = mkdtemp(name); check(p, "mkdtemp"); path = p; }
    ~Fixture() { fs::current_path(cwd); std::error_code ec; fs::remove_all(path, ec); }
};
void menu(EditorState& e, int item) { check(e.mode() == EditorState::Mode::Navigate, "navigate before menu"); e.handle(InputAction::Menu); for (int i=0;i<item;++i) e.handle(InputAction::Down); e.handle(InputAction::Confirm); }
void enable(EditorState& e) { menu(e,8); check(e.mode()==EditorState::Mode::EnableEditing && e.readOnly(), "enable requires confirmation"); e.handle(InputAction::Up); e.handle(InputAction::Confirm); check(!e.readOnly(), "confirmed enable"); }
void bufferTests() {
    TextBuffer b;
    check(b.load("") && b.lineCount()==1 && b.cursor()==0 && !b.dirty(), "empty file");
    check(!b.backspace() && !b.erase() && !b.undo() && !b.redo(), "empty bounds/history");
    check(b.insert("é😀") && b.cursor()==2, "UTF8 insert counts scalars");
    check(b.backspace() && b.utf8()=="é", "backspace entire UTF8 scalar");
    check(b.insert("\nnext") && b.lineCount()==2, "newline splits line");
    b.setCursor(2); check(b.backspace() && b.utf8()=="énext", "backspace joins lines");
    check(b.undo() && b.utf8()=="é\nnext", "undo join");
    b.setCursor(1); check(b.erase() && b.utf8()=="énext", "delete joins lines");
    check(b.undo() && b.redo() && b.utf8()=="énext", "redo join");
    b.markSaved(); check(!b.dirty(), "mark saved"); check(b.undo() && b.dirty(), "undo after save dirty");
    check(b.insert("x") && !b.canRedo(), "new edit clears redo");
    check(b.load("abcd\nx\n\nabcdef"), "movement fixture"); b.setCursor(3);
    b.moveVertical(1); check(b.line()==1 && b.column()==1, "vertical short clamp");
    b.moveVertical(1); check(b.column()==0, "empty line");
    b.moveVertical(1); check(b.column()==3, "preferred column survives empty line");
    b.moveVertical(-100); check(b.line()==0 && b.column()==3, "top clamp");
    for(int i=0;i<20;++i) b.moveHorizontal(1);
    check(b.line()==0 && b.column()==4, "right line boundary");
    for(int i=0;i<20;++i) b.moveHorizontal(-1);
    check(b.column()==0, "left clamp");
    check(b.goToLine("4") && b.line()==3 && b.column()==0, "goto valid");
    for(auto s : {"", "0", "5", "-1", "1x", " 1", "999999999999999999999999999"}) check(!b.goToLine(s) && b.line()==3, "goto invalid retains cursor");
    check(b.load("one é one") && b.find("one",1)==6 && b.find("one",9)==0, "find next and wrap");
    check(b.find("é",0)==4 && !b.find("ONE",0) && !b.find("",0), "UTF8/case-sensitive/empty find");
    const auto before=b.utf8();
    for(auto bad : {std::string("\xc0\xaf"), std::string("\xed\xa0\x80"), std::string("\0",1), std::string("\x01",1)}) check(!b.insert(bad) && !b.load(bad) && b.utf8()==before, "reject invalid text unchanged");
    check(b.load(std::string(TextBuffer::maxBytes,'x')) && !b.insert("y"), "size limit");
}
void ioTests(Fixture& f) {
    const auto p=f.path/"round.sh"; const std::string raw="\xef\xbb\xbf#!/bin/sh\r\né\r\n";
    write(p,raw); chmod(p.c_str(),0751); DocumentData d; std::string error;
    check(DocumentIO::load(p,d,error) && d.crlf && d.bom && d.text=="#!/bin/sh\né\n", "CRLF/BOM normalize");
    check(DocumentIO::serialize(d.text,d.crlf,d.bom)==raw, "byte exact round trip");
    const auto replacement=DocumentIO::serialize(d.text+"echo ok",d.crlf,d.bom);
    check(DocumentIO::save(p,replacement,d.original,error), error.c_str());
    check(read(p)==replacement && read(p.string()+".bak")==raw, "save and backup");
    struct stat st{}; stat(p.c_str(),&st); check((st.st_mode&0777)==0751, "executable permissions");
    check(DocumentIO::save(p,"next",replacement,error) && read(p.string()+".bak")==replacement, "single backup replaced");
    check(!fs::exists(p.string()+".bak.bak"), "no backup chains");
    check(!DocumentIO::save(p,"bad",replacement,error) && read(p)=="next", "external change refusal");
    check(!DocumentIO::save(p,"bad",std::nullopt,error) && read(p)=="next", "unconfirmed overwrite refusal");
    const auto fresh=f.path/"fresh.txt";
    check(DocumentIO::save(fresh,"new\n",std::nullopt,error) && read(fresh)=="new\n", "atomic new file");
    check(!fs::exists(fresh.string()+".bak"), "new file no backup");
    chmod(p.c_str(),0444); check(!DocumentIO::save(p,"bad",std::string("next"),error) && read(p)=="next", "read only save failure"); chmod(p.c_str(),0644);
    fs::remove(p.string()+".bak"); fs::create_directory(p.string()+".bak");
    check(!DocumentIO::save(p,"bad",std::string("next"),error) && read(p)=="next", "backup failure preserves destination");
    fs::remove(p.string()+".bak"); fs::create_symlink(fresh,p.string()+".bak");
    check(!DocumentIO::save(p,"bad",std::string("next"),error) && read(fresh)=="new\n", "backup symlink refusal");
    check(!DocumentIO::save(f.path/"missing"/"file.txt","x",std::nullopt,error), "missing directory fails");
    for (auto& entry:fs::directory_iterator(f.path)) check(entry.path().filename().string().find(".portpad-")!=0,"temporary files cleaned");
    write(f.path/"binary.txt",std::string("abc\0xyz",7)); check(!DocumentIO::load(f.path/"binary.txt",d,error), "binary refused");
    write(f.path/"mixed.txt","a\r\nb\n"); check(!DocumentIO::load(f.path/"mixed.txt",d,error), "mixed endings fail safely");
    write(f.path/"plain.ini","a\nb"); check(DocumentIO::load(f.path/"plain.ini",d,error) && !d.crlf && d.original==d.text,"LF/no final newline");
    check(!DocumentIO::supported("x.bin") && !DocumentIO::supported("x.txt.bak") && DocumentIO::supported("X.GPTK"), "extensions");
}
void mutationAttempts(EditorState& e) {
    const auto before=e.buffer().utf8(); const bool dirty=e.buffer().dirty();
    for(auto command : {EditorCommand::Backspace,EditorCommand::Delete,EditorCommand::Newline,EditorCommand::Tab,EditorCommand::Undo,EditorCommand::Redo}) e.command(command);
    e.insertText("injected\ntext");
    check(e.buffer().utf8()==before && e.buffer().dirty()==dirty, "read only direct mutations blocked");
    e.handle(InputAction::Confirm); check(!e.textEntry() && !e.keyboardVisible(), "read only VK insertion blocked");
    e.handle(InputAction::Confirm); // dismiss notice
    check(e.buffer().utf8()==before && e.readOnly(), "confirm shortcut cannot unlock");
}
void stateTests(Fixture& f) {
    const auto p=f.path/"state.txt"; write(p,"abc\n\nabc"); std::string error; EditorState e;
    check(e.open(p,error) && e.readOnly(), "view default");
    mutationAttempts(e);
    check(!e.menuAvailable(0)&&!e.menuAvailable(1)&&!e.menuAvailable(2)&&!e.menuAvailable(3), "clean viewer disables writes/history");
    e.command(EditorCommand::SaveAs); check(e.mode()==EditorState::Mode::Navigate,"view SaveAs shortcut blocked");
    e.command(EditorCommand::Save); check(!fs::exists(p.string()+".bak"),"view Save shortcut blocked");
    e.handle(InputAction::Down); check(e.buffer().line()==1,"viewer navigation");
    e.command(EditorCommand::Find); e.insertText("abc"); e.command(EditorCommand::Newline);
    check(e.buffer().cursor()==5 && e.highlighted(5) && !e.buffer().dirty(),"readonly search prompt doesn't mutate document");
    e.handle(InputAction::Details); check(e.buffer().cursor()==0,"find next wraps in viewer");
    e.command(EditorCommand::GoToLine); e.insertText("3"); e.command(EditorCommand::Newline); check(e.buffer().line()==2,"viewer goto");
    e.handle(InputAction::Menu); e.handle(InputAction::Details); check(e.notice().active(),"full path available"); e.handle(InputAction::Back); e.handle(InputAction::Back);
    menu(e,8); e.command(EditorCommand::Newline); e.insertText("no"); e.handle(InputAction::Confirm); check(e.readOnly() && !e.buffer().dirty(),"default No confirmation");
    enable(e); e.handle(InputAction::Confirm); e.insertText("X"); e.command(EditorCommand::Newline); e.insertText("é"); e.handle(InputAction::Menu);
    const auto edited=e.buffer().utf8(); check(e.buffer().dirty(),"editing enabled");
    menu(e,8); check(e.readOnly() && e.buffer().dirty() && e.buffer().utf8()==edited,"lock retains edits"); mutationAttempts(e);
    check(e.menuAvailable(0)&&e.menuAvailable(1)&&!e.menuAvailable(2),"dirty viewer can save but not undo");
    menu(e,2); check(e.mode()==EditorState::Mode::Menu && e.buffer().utf8()==edited,"readonly menu undo disabled"); e.handle(InputAction::Back);
    e.requestClose(EditorState::Exit::Close); e.handle(InputAction::Confirm); check(e.loaded() && e.buffer().dirty(),"close cancel retains edits");
    e.command(EditorCommand::Save); check(!e.buffer().dirty() && read(p)==edited && e.readOnly(),"dirty readonly save");
    enable(e); e.command(EditorCommand::Undo); check(e.buffer().dirty(),"resume undo works");
    menu(e,8); e.requestClose(EditorState::Exit::Close); e.handle(InputAction::Up); e.handle(InputAction::Confirm);
    check(!e.loaded() && e.takeExit()==EditorState::Exit::Close && read(p)==edited,"discard readonly dirty leaves disk");
    check(e.open(p,error,false),"direct editable open"); e.handle(InputAction::Confirm); e.insertText("changed"); e.handle(InputAction::Menu);
    write(p,"external"); const auto unsaved=e.buffer().utf8(); e.command(EditorCommand::Save);
    check(e.notice().active() && e.buffer().dirty() && e.buffer().utf8()==unsaved && read(p)=="external","failed save keeps buffer dirty"); e.handle(InputAction::Back);
    menu(e,8); e.command(EditorCommand::SaveAs);
    // Default existing filename must require explicit overwrite confirmation.
    e.command(EditorCommand::Newline); check(e.mode()==EditorState::Mode::Overwrite,"SaveAs existing confirmation");
    e.handle(InputAction::Confirm); check(e.mode()==EditorState::Mode::Prompt && read(p)=="external","default cancel overwrite");
    e.command(EditorCommand::Newline); e.handle(InputAction::Up); e.handle(InputAction::Confirm);
    check(!e.buffer().dirty() && read(p)==unsaved && read(p.string()+".bak")=="external","dirty readonly SaveAs confirmed");
}
void controllerWorkflow(Fixture& f) {
    const auto p=f.path/"controller.ini"; write(p,""); EditorState e; std::string error;
    check(e.open(p,error,false),"controller fixture"); e.handle(InputAction::Confirm);
    e.handle(InputAction::Confirm); // q
    e.handle(InputAction::PageUp); e.handle(InputAction::Confirm); // Q
    // Enter key: row 5, column 3.
    for(int i=0;i<4;++i) e.handle(InputAction::Down);
    e.handle(InputAction::Right); e.handle(InputAction::Right); e.handle(InputAction::Confirm);
    check(e.buffer().utf8()=="qQ\n","controller insertion/newline");
    e.handle(InputAction::Back); check(e.buffer().utf8()=="qQ","controller backspace joins");
    e.handle(InputAction::Menu); menu(e,0); check(read(p)=="qQ" && !e.buffer().dirty(),"controller menu save");
    menu(e,1); // Replace suggested filename using controller backspace.
    for(std::size_t i=0;i<std::string("controller.ini").size();++i) e.handle(InputAction::Back);
    check(e.mode()==EditorState::Mode::Prompt,"empty filename still input");
    e.insertText("copy.ini"); // Desktop text input shares the prompt with controller Done.
    for(int i=0;i<4;++i) e.handle(InputAction::Down);
    for(int i=0;i<3;++i) e.handle(InputAction::Right);
    e.handle(InputAction::Confirm);
    check(e.path()==f.path/"copy.ini" && read(e.path())=="qQ" && !e.buffer().dirty(),"SaveAs new path and controller Done");
}
void keyboardTests() {
    VirtualKeyboard k; check(k.handle(InputAction::Confirm)->text=="q","keyboard letters");
    k.handle(InputAction::PageUp); check(k.handle(InputAction::Confirm)->text=="Q" && k.handle(InputAction::Confirm)->text=="q","temporary shift");
    k.handle(InputAction::PageDown); std::string symbols;
    for(const auto& row:k.rows()) for(const auto& key:row) symbols+=key.text;
    for(char c:std::string("-_/\\.,:;=+*?!\"'()[]{}<>$%&|@#~")) check(symbols.find(c)!=std::string::npos,"required shell symbol");
    k.reset(true); check(k.handle(InputAction::Confirm)->text=="1","numeric layer");
    for(int i=0;i<100;++i) { k.handle(InputAction::Right); k.handle(InputAction::Down); k.handle(InputAction::PageDown); check(k.column()<k.rows()[k.row()].size(),"keyboard selection bounds"); }
}
void browserTests(Fixture& f) {
    auto dir=f.path/"browser"; fs::create_directory(dir); write(dir/"a.txt","hello"); write(dir/"b.txt",std::string("\0",1)); write(dir/"c.bin","binary"); fs::current_path(dir);
    AppState app; app.handle(InputAction::Confirm); app.handle(InputAction::Down); app.handle(InputAction::Confirm);
    check(app.fileActionActive() && app.fileActionSelected()==0,"browser View Edit Cancel default View");
    app.handle(InputAction::Back); check(!app.fileActionActive() && app.browser().path()==dir,"cancel preserves browser directory");
    app.handle(InputAction::Confirm); app.handle(InputAction::Confirm); check(app.editorActive() && app.editor().readOnly(),"View opens shared readonly editor");
    app.handle(InputAction::Back); app.handle(InputAction::Confirm); app.handle(InputAction::Down); app.handle(InputAction::Confirm);
    check(app.editorActive() && !app.editor().readOnly(),"Edit opens writable editor");
    app.handle(InputAction::Back); app.handle(InputAction::Down); app.handle(InputAction::Confirm); check(!app.fileActionActive() && app.dialog().active(),"binary text has no edit option");
    app.handle(InputAction::Back); app.handle(InputAction::Down); app.handle(InputAction::Confirm); check(!app.fileActionActive() && app.dialog().active(),"unsupported file safe behavior");
}
int main() { try { Fixture f; bufferTests(); ioTests(f); stateTests(f); controllerWorkflow(f); keyboardTests(); browserTests(f); std::cout<<"Editor, I/O, read-only, keyboard and browser tests passed\n"; } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
