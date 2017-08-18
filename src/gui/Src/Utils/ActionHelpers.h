#ifndef ACTIONHELPERS_H
#define ACTIONHELPERS_H

#include <QAction>

template<class Base>
class ActionHelperBase
{
private:
    Base* getBase()
    {
        return static_cast<Base*>(this);
    }

    virtual QWidget* getParent() = 0;
    virtual bool connectWithObject() = 0;

private:
    struct ActionShortcut
    {
        QAction* action;
        QString shortcut;

        inline ActionShortcut(QAction* action, const char* shortcut)
            : action(action),
              shortcut(shortcut)
        {
        }
    };

public:
    virtual void updateShortcuts()
    {
        for(const auto & actionShortcut : actionShortcutPairs)
            actionShortcut.action->setShortcut(ConfigShortcut(actionShortcut.shortcut));
    }

private:

    inline QAction* connectAction(QAction* action, const char* slot)
    {
        QObject::connect(action, SIGNAL(triggered(bool)), getBase(), slot);
        return action;
    }

    template<class T> // lambda or base member pointer
    inline QAction* connectAction(QAction* action, T callback)
    {
        if(connectWithObject())
            QObject::connect(action, &QAction::triggered, getBase(), callback);
        else
            QObject::connect(action, &QAction::triggered, callback);
        return action;
    }

    virtual inline QAction* connectShortcutAction(QAction* action, const char* shortcut)
    {
        actionShortcutPairs.push_back(ActionShortcut(action, shortcut));
        action->setShortcut(ConfigShortcut(shortcut));
        action->setShortcutContext(Qt::WidgetShortcut);
        getParent()->addAction(action);
        return action;
    }

    inline QAction* connectMenuAction(QMenu* menu, QAction* action)
    {
        menu->addAction(action);
        return action;
    }
protected:

    inline QMenu* makeMenu(const QString & title)
    {
        return new QMenu(title, getParent());
    }

    inline QMenu* makeMenu(const QIcon & icon, const QString & title)
    {
        QMenu* menu = new QMenu(title, getParent());
        menu->setIcon(icon);
        return menu;
    }

    template<typename T>
    inline QAction* makeAction(const QString & text, T slot)
    {
        return connectAction(new QAction(text, getParent()), slot);
    }

    template<typename T>
    inline QAction* makeAction(const QIcon & icon, const QString & text, T slot)
    {
        return connectAction(new QAction(icon, text, getParent()), slot);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(icon, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(text, slot));
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(icon, text, slot));
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, icon, text, slot), shortcut);
    }

private:
    std::vector<ActionShortcut> actionShortcutPairs;

private:
    template<typename Base>
    friend class ActionHelper;

    template<typename Base, typename>
    friend class ProxyActionHelper;
};

template<typename Base>
class ActionHelper : public ActionHelperBase<Base>
{
private:
    virtual QWidget* getParent() override
    {
        return getBase();
    }

    virtual bool connectWithObject() override
    {
        return true;
    }
};


template<typename Base, typename Parent>
class ProxyActionHelper : public ActionHelperBase<Base>
{
public:
    ProxyActionHelper(Parent* parent)
        : parentWidget(parent)
    {
    }

private:
    virtual QWidget* getParent() override
    {
        return parentWidget;
    }

    virtual bool connectWithObject() override
    {
        return false;
    }

    virtual inline QAction* connectShortcutAction(QAction* action, const char* shortcut) override
    {
        return ((ActionHelperBase<Parent>*)parentWidget)->connectShortcutAction(action, shortcut);
    }

protected:
    Parent* parentWidget;
};

#endif
