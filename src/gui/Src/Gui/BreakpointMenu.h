#ifndef BREAKPOINTMENU_H
#define BREAKPOINTMENU_H

template<typename Parent>
class BreakpointMenu : public QObject, public ProxyActionHelper<BreakpointMenu<Parent>, Parent>
{
public:
    typedef std::function<duint()> SelectionGetter;

    BreakpointMenu(Parent* parent, SelectionGetter selectionGetter);

    void addMenu(MenuBuilder* menuBuilder);

    void toggleInt3BPActionSlot();
    void editSoftBpActionSlot();
    void toggleHwBpActionSlot();
    void setHwBpOnSlot0ActionSlot();
    void setHwBpOnSlot1ActionSlot();
    void setHwBpOnSlot2ActionSlot();
    void setHwBpOnSlot3ActionSlot();
    void setHwBpAt(duint va, int slot);
private:
    SelectionGetter getCurrentVa;
};

template<typename Parent>
BreakpointMenu<Parent>::BreakpointMenu(Parent* parent, SelectionGetter selectionGetter)
    : getCurrentVa(selectionGetter), ProxyActionHelper(parent)
{
}

template<typename Parent>
void BreakpointMenu<Parent>::addMenu(MenuBuilder* menuBuilder)
{
    QAction* toggleBreakpointAction = makeShortcutAction(DIcon("breakpoint_toggle.png"), tr("Toggle"), [this]() { toggleInt3BPActionSlot(); }, "ActionToggleBreakpoint");
    QAction* editSoftwareBreakpointAction = makeShortcutAction(DIcon("breakpoint_edit_alt.png"), tr("Edit"), [this]() { editSoftBpActionSlot(); }, "ActionEditBreakpoint");
    QAction* setHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_execute.png"), tr("Set Hardware on Execution"), [this]() { toggleHwBpActionSlot(); }, "ActionSetHwBpE");
    QAction* removeHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_remove.png"), tr("Remove Hardware"), [this]() { toggleHwBpActionSlot(); }, "ActionRemoveHwBp");

    QMenu* replaceSlotMenu = makeMenu(DIcon("breakpoint_execute.png"), tr("Set Hardware on Execution"));
    QAction* replaceSlot0Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot1.png"), tr("Replace Slot 0 (Free)"), [this]() { setHwBpOnSlot0ActionSlot(); });
    QAction* replaceSlot1Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot2.png"), tr("Replace Slot 1 (Free)"), [this]() { setHwBpOnSlot1ActionSlot(); });
    QAction* replaceSlot2Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot3.png"), tr("Replace Slot 2 (Free)"), [this]() { setHwBpOnSlot2ActionSlot(); });
    QAction* replaceSlot3Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot4.png"), tr("Replace Slot 3 (Free)"), [this]() { setHwBpOnSlot3ActionSlot(); });

    menuBuilder->addMenu(makeMenu(DIcon("breakpoint.png"), tr("Breakpoint")), [ = ](QMenu * menu)
    {
        BPXTYPE bpType = DbgGetBpxTypeAt(getCurrentVa());
        if((bpType & bp_normal) == bp_normal)
            menu->addAction(editSoftwareBreakpointAction);

        menu->addAction(toggleBreakpointAction);

        if((bpType & bp_hardware) == bp_hardware)
        {
            menu->addAction(removeHwBreakpointAction);
        }
        else
        {
            BPMAP bpList;
            DbgGetBpList(bp_hardware, &bpList);

            //get enabled hwbp count
            int enabledCount = bpList.count;
            for(int i = 0; i < bpList.count; i++)
                if(!bpList.bp[i].enabled)
                    enabledCount--;

            if(enabledCount < 4)
            {
                menu->addAction(setHwBreakpointAction);
            }
            else
            {
                for(int i = 0; i < 4; i++)
                {
                    switch(bpList.bp[i].slot)
                    {
                    case 0:
                        replaceSlot0Action->setText(tr("Replace Slot %1 (0x%2)").arg(1).arg(ToPtrString(bpList.bp[i].addr)));
                        break;
                    case 1:
                        replaceSlot1Action->setText(tr("Replace Slot %1 (0x%2)").arg(2).arg(ToPtrString(bpList.bp[i].addr)));
                        break;
                    case 2:
                        replaceSlot2Action->setText(tr("Replace Slot %1 (0x%2)").arg(3).arg(ToPtrString(bpList.bp[i].addr)));
                        break;
                    case 3:
                        replaceSlot3Action->setText(tr("Replace Slot %1 (0x%2)").arg(4).arg(ToPtrString(bpList.bp[i].addr)));
                        break;
                    default:
                        break;
                    }
                }
                menu->addMenu(replaceSlotMenu);
            }
            if(bpList.count)
                BridgeFree(bpList.bp);
        }
        return true;
    });
}

template<typename Parent>
void BreakpointMenu<Parent>::toggleInt3BPActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = getCurrentVa();
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        wCmd = "bc " + ToPtrString(wVA);
    }
    else
    {
        if(DbgFunctions()->IsDepEnabled() && !DbgFunctions()->MemIsCodePage(wVA, false))
        {
            QMessageBox msgyn(QMessageBox::Warning, tr("Current address is not executable"),
                              tr("Setting software breakpoint here may result in crash. Do you really want to continue?"), QMessageBox::Yes | QMessageBox::No, parentWidget);
            msgyn.setWindowIcon(DIcon("compile-warning.png"));
            msgyn.setParent(parentWidget, Qt::Dialog);
            msgyn.setWindowFlags(msgyn.windowFlags() & (~Qt::WindowContextHelpButtonHint));
            if(msgyn.exec() == QMessageBox::No)
                return;
        }
        wCmd = "bp " + ToPtrString(wVA);
    }

    DbgCmdExec(wCmd.toUtf8().constData());
    //emit Disassembly::repainted();
}

template<typename Parent>
void BreakpointMenu<Parent>::editSoftBpActionSlot()
{
    Breakpoints::editBP(bp_normal, ToHexString(getCurrentVa()), parentWidget);
}

template<typename Parent>
void BreakpointMenu<Parent>::toggleHwBpActionSlot()
{
    duint wVA = getCurrentVa();
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_hardware) == bp_hardware)
    {
        wCmd = "bphwc " + ToPtrString(wVA);
    }
    else
    {
        wCmd = "bphws " + ToPtrString(wVA);
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}


template<typename Parent>
void BreakpointMenu<Parent>::setHwBpOnSlot0ActionSlot()
{
    setHwBpAt(getCurrentVa(), 0);
}

template<typename Parent>
void BreakpointMenu<Parent>::setHwBpOnSlot1ActionSlot()
{
    setHwBpAt(getCurrentVa(), 1);
}

template<typename Parent>
void BreakpointMenu<Parent>::setHwBpOnSlot2ActionSlot()
{
    setHwBpAt(getCurrentVa(), 2);
}

template<typename Parent>
void BreakpointMenu<Parent>::setHwBpOnSlot3ActionSlot()
{
    setHwBpAt(getCurrentVa(), 3);
}

template<typename Parent>
void BreakpointMenu<Parent>::setHwBpAt(duint va, int slot)
{
    int wI = 0;
    int wSlotIndex = -1;
    BPMAP wBPList;
    QString wCmd = "";

    DbgGetBpList(bp_hardware, &wBPList);

    // Find index of slot slot in the list
    for(wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].slot == (unsigned short)slot)
        {
            wSlotIndex = wI;
            break;
        }
    }

    if(wSlotIndex < 0) // Slot not used
    {
        wCmd = "bphws " + ToPtrString(va);
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    else // Slot used
    {
        wCmd = "bphwc " + ToPtrString((duint)(wBPList.bp[wSlotIndex].addr));
        DbgCmdExec(wCmd.toUtf8().constData());

        Sleep(200);

        wCmd = "bphws " + ToPtrString(va);
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

#endif // BREAKPOINTMENU_H
