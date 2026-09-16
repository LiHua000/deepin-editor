// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "insertblockbytextcommond.h"
#include "inserttextundocommand.h"
#include <QApplication>
#include "dtextedit.h"
#include "editwrapper.h"
#include "../widgets/window.h"
#include "../widgets/bottombar.h"

InsertBlockByTextCommand::InsertBlockByTextCommand(const QString &text,TextEdit *edit,EditWrapper* wrapper):
    m_text(text),
    m_edit(edit),
    m_wrapper(wrapper)
{
    if(nullptr == m_edit || m_text.isEmpty() || nullptr == m_wrapper)
        return;

    auto cursor = m_edit->textCursor();
    if(cursor.hasSelection()){
        m_selected = cursor.selectedText();
        m_insertPos = std::min(cursor.anchor(),cursor.position());
    }
}

InsertBlockByTextCommand::~InsertBlockByTextCommand()
{

}

void InsertBlockByTextCommand::redo()
{
    treat(true);
    insertByBlock();
    treat(false, true);
}

void InsertBlockByTextCommand::undo()
{
    treat(true);

    auto cursor = m_edit->textCursor();
    cursor.setPosition(m_delPos);
    cursor.setPosition(m_delPos - m_text.size(),QTextCursor::KeepAnchor);
    cursor.deleteChar();

    if(!m_selected.isEmpty()){
        cursor.setPosition(m_insertPos);
        cursor.insertText(m_selected);
    }

    treat(false, false);
}

void InsertBlockByTextCommand::treat(bool isStart, bool isInsert)
{
    if(m_wrapper!=nullptr){

        Window* window = m_wrapper->window();
        BottomBar* bar = m_wrapper->bottomBar();
        if(window){
            window->setPrintEnabled(!isStart);
        }
        if(bar){
            bar->setChildEnabled(!isStart);
        }
    }

    if(!isStart) {
        // Reconnect cursorPositionChanged
        QObject::connect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);

        // Reconnect textChanged
        QObject::connect(m_edit, &QPlainTextEdit::textChanged, m_edit, &TextEdit::updateLeftAreaWidget);
        QObject::connect(m_edit, &QPlainTextEdit::textChanged, m_edit, [edit = m_edit, wrapper = m_wrapper]() {
            wrapper->UpdateBottomBarWordCnt(edit->characterCount());
        });

        // Reconnect contentsChange (onTextContentChanged left connected — no-op during block insert)
        QObject::connect(m_edit->document(), &QTextDocument::contentsChange, m_edit, &TextEdit::updateMark);
        QObject::connect(m_edit->document(), &QTextDocument::contentsChange, m_edit, &TextEdit::checkBookmarkLineMove);

        // Unified state refresh after reconnection
        m_edit->updateLeftAreaWidget();
        m_edit->cursorPositionChanged();
        if (m_wrapper)
            m_wrapper->UpdateBottomBarWordCnt(m_edit->characterCount());
        int insertPos = m_delPos - m_text.size();
        if (insertPos < 0)
            insertPos = 0;
        if (isInsert) {
            m_edit->updateMark(insertPos, 0, m_text.size());
            m_edit->checkBookmarkLineMove(insertPos, 0, m_text.size());
        } else {
            m_edit->updateMark(insertPos, m_text.size(), 0);
            m_edit->checkBookmarkLineMove(insertPos, m_text.size(), 0);
        }
    } else {
        // Disconnect cursorPositionChanged
        QObject::disconnect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);

        // Disconnect textChanged (all connections to m_edit)
        QObject::disconnect(m_edit, &QPlainTextEdit::textChanged, m_edit, nullptr);

        // Disconnect contentsChange — only the expensive public slots
        // (onTextContentChanged is a private slot and a no-op during block insert, left connected)
        QObject::disconnect(m_edit->document(), &QTextDocument::contentsChange, m_edit, &TextEdit::updateMark);
        QObject::disconnect(m_edit->document(), &QTextDocument::contentsChange, m_edit, &TextEdit::checkBookmarkLineMove);
    }
}

void InsertBlockByTextCommand::insertByBlock()
{
    auto cursor = m_edit->textCursor();
    int block = 1 * 1024 * 1024 ;
    int size = m_text.size();
    if(size > block){
        int n = size / (block);
        int y = size % (block);
        int k=0;
        for(;k<n;k++){
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,block);
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
        if(y){
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,y);
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
    }
    m_delPos = cursor.position();
}
