// SPDX-FileCopyrightText: 2017 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../common/utils.h"
#include "../common/settings.h"
#include "ddropdownmenu.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <DApplication>
#include <QPainter>
#include <DSettingsOption>
#include <QDebug>
#include <DFontSizeManager>
#include <DLabel>
#include <DApplicationHelper>
#include <QtSvg/QSvgRenderer>

using namespace Dtk::Core;

// 不同布局模式(紧凑)
const int s_DDropdownMenuHeight = 28;
const int s_DDropdownMenuHeightCompact = 20;

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent)
    , m_textLabel(new DLabel(this))
    , m_arrowLabel(new DLabel(this))
    , m_menu(new DMenu)
{
    // 更新单独添加的高亮格式文件
    m_Repository.addCustomSearchPath(KF5_HIGHLIGHT_PATH);

    //设置文字标签属性
    m_textLabel->setFocusPolicy(Qt::StrongFocus);
    m_textLabel->installEventFilter(this);
    m_textLabel->setText(m_text);

    //设置箭头图标
    QString theme =  (DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Background).lightness() < 128) ? "dark" : "light";
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    // 根据当前显示缩放转换图片
    qreal scaled = this->devicePixelRatioF();
    QSvgRenderer svg_render(arrowSvgPath);
    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);
    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));
    m_arrowPixmap = pixmap;
    m_arrowLabel->setPixmap(m_arrowPixmap);
    m_arrowLabel->setFixedSize(8, 5);
    m_arrowLabel->installEventFilter(this);

    //设置字体
    int fontsize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);
    m_textLabel->setFont(m_font);

     //添加布局
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_textLabel);
    layout->addWidget(m_arrowLabel);
    layout->setContentsMargins(10, 0, 2, 0);
    layout->setSpacing(2);
    this->setLayout(layout);

    connect(this, &DDropdownMenu::requestContextMenu, this, &DDropdownMenu::slotRequestMenu);

    //设置字体自适应大小
    //设置界面大小根据内容大小自适应 梁卫东 2020.7.7
    connect(qApp,&DApplication::fontChanged,this,&DDropdownMenu::OnFontChangedSlot);

#ifdef DTKWIDGET_CLASS_DSizeMode
    setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_DDropdownMenuHeightCompact : s_DDropdownMenuHeight);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [this](){
        setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_DDropdownMenuHeightCompact : s_DDropdownMenuHeight);
    });
#else
    setFixedHeight(s_DDropdownMenuHeight);
#endif
}

DDropdownMenu::~DDropdownMenu()
{
    deleteMenuActionGroup();
    deleteMenu();
}

void DDropdownMenu::setFontEx(const QFont& font)
{
    m_textLabel->setFont(font);
    m_font = font;
}

void DDropdownMenu::setCurrentAction(QAction *pAct)
{
    if(pAct){
        QList<QAction*> menuList = m_menu->actions();
        pAct->setChecked(true);
        for (int i = 0; i < menuList.size(); i++) {
            QList<QAction*> acts = menuList[i]->menu()->actions();
            for (int j = 0; j < acts.size(); j++) {
                if(acts[j] != pAct) acts[j]->setChecked(false);
            }
        }
        setText(pAct->text());
    }
}

void DDropdownMenu::setCurrentTextOnly(const QString &name)
{
  // QList<QAction*> menuList = m_menu->actions();

//   for (int i = 0; i < menuList.size(); i++) {
//       if(menuList[i]->menu()){
//           QList<QAction*> acts = menuList[i]->menu()->actions();
//           if(acts.size() == 0) continue;
//           for (int j = 0; j < acts.size(); j++) {
//           if(acts[j]->text() != name){
//               acts[j]->setCheckable(false);
//               acts[j]->setChecked(false);
//           }
//           else{
//               acts[j]->setCheckable(true);
//               acts[j]->setChecked(true);
//           }
//        }
//      }
//   }
   for(auto ac:m_menu->actions()){
       setCheckedExclusive(ac,name);
   }

   setText(name);
}


void DDropdownMenu::setCheckedExclusive(QAction* action,const QString& name)
{
    if(nullptr == action){
        return;
    }

    if(action->menu()){
        for(auto ac:action->menu()->actions()){
            setCheckedExclusive(ac,name);
        }
    }
    else {
        if(action->text() != name){
            action->setCheckable(false);
            action->setChecked(false);
        }
        else{
            action->setCheckable(true);
            action->setChecked(true);
        }
    }
}

void DDropdownMenu::slotRequestMenu(bool request)
{
    if (request) {
        //如果鼠标点击清除ｆｏｃｕｓ
        m_textLabel->clearFocus();
    }
    QPoint center = this->mapToGlobal(this->rect().center());
    int menuHeight = m_menu->sizeHint().height();
    int menuWidth = m_menu->sizeHint().width();
    center.setY(center.y() - menuHeight - this->rect().height() / 2);
    center.setX(center.x() - menuWidth / 2);
    m_menu->move(center);
    m_menu->exec();
    //清除ｆｏｃｕｓ
    m_textLabel->clearFocus();
    QEvent event(QEvent::HoverLeave);
    QApplication::sendEvent(m_textLabel, &event);
    emit sigSetTextEditFocus();
}

void DDropdownMenu::setText(const QString &text)
{
    m_text = text;
    m_textLabel->setText(m_text);
}

void DDropdownMenu::setMenu(DMenu *menu)
{
    deleteMenu();
    m_menu = menu;
}

void DDropdownMenu::deleteMenu()
{
    if (m_menu != nullptr) {
        delete m_menu;
        m_menu = nullptr;
    }
}

void DDropdownMenu::setMenuActionGroup(QActionGroup *actionGroup)
{
    deleteMenuActionGroup();
    m_actionGroup = actionGroup;
}

void DDropdownMenu::deleteMenuActionGroup()
{
    if (m_actionGroup != nullptr) {
        delete m_actionGroup;
        m_actionGroup = nullptr;
    }
}

void DDropdownMenu::setTheme(const QString &theme)
{
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    // 根据当前显示缩放转换图片
    qreal scaled = this->devicePixelRatioF();
    QSvgRenderer svg_render(arrowSvgPath);

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    m_arrowPixmap = pixmap;
    m_arrowLabel->setPixmap(m_arrowPixmap);
}

void DDropdownMenu::setChildrenFocus(bool ok)
{
    if(ok)  m_textLabel->setFocusPolicy(Qt::StrongFocus);
    else  {
        m_textLabel->clearFocus();
        m_textLabel->setFocusPolicy(Qt::NoFocus);
    }
}

void DDropdownMenu::setRequestMenu(bool request)
{
    isRequest = request;
}

QWidget *DDropdownMenu::getButton()
{
    return m_textLabel;
}

QString DDropdownMenu::getCurrentText() const
{
    return m_text;
}

DDropdownMenu *DDropdownMenu::createEncodeMenu()
{
    DDropdownMenu *m_pEncodeMenu = new DDropdownMenu();
    DMenu* m_pMenu = new DMenu();

    auto groupEncodeVec = Utils::getSupportEncoding();
    if (!groupEncodeVec.isEmpty()) {
        int cnt = groupEncodeVec.size();
        for (int i = 0; i < cnt;i++) {
            QMenu* groupMenu = new QMenu(QObject::tr(groupEncodeVec[i].first.toLocal8Bit().data()));
             foreach(QString var, groupEncodeVec[i].second)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               if(act->text() == "UTF-8") {
                   m_pEncodeMenu->m_pActUtf8 = act;
                   act->setCheckable(true);
                   act->setChecked(true);
               }else {
                   act->setCheckable(false);
               }
             }

            m_pMenu->addMenu(groupMenu);
        }
    }

    connect(m_pMenu, &DMenu::triggered, m_pEncodeMenu,[m_pEncodeMenu](QAction *action) {
        //编码内容改变触发内容改变和信号发射 梁卫东 2020.7.7
        if (m_pEncodeMenu->m_text != action->text()) {
            emit m_pEncodeMenu->currentActionChanged(action);
        }
    });

    m_pEncodeMenu->setText("UTF-8");
    m_pEncodeMenu->setMenu(m_pMenu);

    return  m_pEncodeMenu;
}

DDropdownMenu *DDropdownMenu::createHighLightMenu()
{
    DDropdownMenu *m_pHighLightMenu = new DDropdownMenu();
    DMenu *m_pMenu = new DMenu;
    QAction *noHlAction = m_pMenu->addAction(tr("None"));
    noHlAction->setCheckable(true);

    QActionGroup* m_pActionGroup = new QActionGroup(m_pMenu);
    m_pActionGroup->setExclusive(true);
    m_pActionGroup->addAction(noHlAction);

    DMenu *pSubMenu = nullptr;
    QString currentGroup;

    bool intel = true;
    for (KSyntaxHighlighting::Definition def : m_pHighLightMenu->m_Repository.definitions()) {

        if(def.translatedName()=="Intel x86 (NASM)"&&intel)
        {
            intel = false;
            continue;
        }
        if (def.isHidden()) {
            continue;
        }

        if (currentGroup != def.section()) {
            currentGroup = def.section();
            pSubMenu = m_pMenu->addMenu(def.translatedSection());
        }

        if (!pSubMenu) {
            continue;
        }

        QAction* action = pSubMenu->addAction(def.translatedName());
        action->setCheckable(true);
        action->setText(def.name());
        m_pActionGroup->addAction(action);
    }

    // 转发选中“None“无高亮选项的信号
    connect(noHlAction, &QAction::triggered, m_pHighLightMenu, [noHlAction, m_pHighLightMenu] (bool checked) {
        if (checked) {
            emit m_pHighLightMenu->currentActionChanged(noHlAction);
        }
    });

    connect(m_pActionGroup, &QActionGroup::triggered, m_pHighLightMenu, [m_pHighLightMenu] (QAction *action) {
        const auto defName = action->text();
        const auto def = m_pHighLightMenu->m_Repository.definitionForName(defName);
        if (def.isValid() && m_pHighLightMenu->m_text != action->text()) {
            emit m_pHighLightMenu->currentActionChanged(action);
        }
        else {
            m_pHighLightMenu->setText(tr("None"));
        }
        
    });

    m_pHighLightMenu->setText(tr("None"));
    m_pHighLightMenu->setMenu(m_pMenu);
    m_pHighLightMenu->setMenuActionGroup(m_pActionGroup);

    return m_pHighLightMenu;
}

void DDropdownMenu::updateAppearance()
{
    DPalette dpalette = DApplicationHelper::instance()->palette(m_textLabel);
    QPalette pal = m_textLabel->palette();

    if(m_bPressed){
        QColor textColor = dpalette.color(DPalette::Highlight);
        pal.setColor(QPalette::WindowText, textColor);
        m_textLabel->setPalette(pal);
        QString color = textColor.name(QColor::HexRgb);
        m_arrowLabel->setPixmap(setSvgColor(color));
    } else {
        QColor textColor = dpalette.color(DPalette::WindowText);
        pal.setColor(QPalette::WindowText, textColor);
        m_textLabel->setPalette(pal);
        m_arrowLabel->setPixmap(m_arrowPixmap);
    }
}

void DDropdownMenu::OnFontChangedSlot(const QFont &font)
{
    m_font = font;
    int fontsize =DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T8);
    m_font.setPixelSize(fontsize);
    m_textLabel->setFont(m_font);
}


bool DDropdownMenu::eventFilter(QObject *object, QEvent *event)
{
    if(object == m_textLabel || object == m_arrowLabel){
        if(event->type() == QEvent::KeyPress){
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            //QString key = Utils::getKeyshortcut(keyEvent);
            if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space)        //按下enter展开列表
            {
                //if(isRequest){
                    Q_EMIT requestContextMenu(false);
                //}
                return true;
            }
            return false;
        }

        if(event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                m_bPressed = true;
                //重新绘制 点击改变前景色
                updateAppearance();
                return true;
            }

            if(mouseEvent->button() == Qt::RightButton){
                return true;
            }
        }

        if(event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                m_bPressed = false;
                updateAppearance();
                if (isEnabled()) {
                    Q_EMIT requestContextMenu(true);
                }
                m_textLabel->clearFocus();
            }
            return true;
        }
    }
    return QFrame::eventFilter(object,event);
}

QPixmap DDropdownMenu::setSvgColor(QString color)
{
    //设置图标颜色
    QString path = QString(":/images/arrow_dark.svg");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    QDomDocument doc;
    doc.setContent(data);
    file.close();
    QDomElement elem = doc.documentElement();
    SetSVGBackColor(elem, "fill", color);

    //装换图片
    //int scaled =qApp->devicePixelRatio() == 1.25 ? 2 : 1;
    qreal scaled = this->devicePixelRatioF();
    QSvgRenderer svg_render(doc.toByteArray());

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    return pixmap;
}

void DDropdownMenu::SetSVGBackColor(QDomElement &elem, QString strattr, QString strattrval)
{

    if (elem.tagName().compare("g") == 0 && elem.attribute("id").compare("color") == 0)
    {
        QString before_color = elem.attribute(strattr);
        elem.setAttribute(strattr, strattrval);
    }
    for (int i = 0; i < elem.childNodes().count(); i++)
    {
        if (!elem.childNodes().at(i).isElement()) continue;
        QDomElement element = elem.childNodes().at(i).toElement();
        SetSVGBackColor(element, strattr, strattrval);
    }
}
