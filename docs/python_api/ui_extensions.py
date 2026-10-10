import renderccccc as rd
import qrenderccccc as qrd
from typing import List


def check_draw(best_size: int, action: rd.ActionDescription):
    if action.flags & rd.ActionFlags.Drawcall:
        size = action.numIndices * action.numInstances
        if size > best_size:
            return action.eventId, size
    return 0, 0


def find_largest_draw(best_size: int, actions: List[rd.ActionDescription]):
    ret = 0
    for action in actions:
        result = check_draw(best_size, action)

        if result[0] == 0:
            result = find_largest_draw(best_size, action.children)

        if result[0] > 0:
            ret, best_size = result

    return ret, best_size


def open_window(pyrenderccccc: qrd.CaptureContext, data):
    mqt = pyrenderccccc.Extensions().GetMiniQtHelper()

    top = mqt.CreateToplevelWidget("Scavenger Hunt")

    group = mqt.CreateGroupBox(False)
    mqt.SetWidgetText(group, "Exciting scavenger hunt!")

    label = mqt.CreateLabel()
    mqt.SetWidgetText(label, "Guess the biggest draw!")

    eid, size = find_largest_draw(0, pyrenderccccc.CurRootActions())

    def do_guess(pyrenderccccc, widget, text):
        print(f"Spoiler: largest draw is {eid}, it drew {size} indices")

        if pyrenderccccc.CurEvent() == eid:
            msg = "You found it!"
        elif pyrenderccccc.CurEvent() < eid:
            msg = "The largest draw is later in the capture..."
        else:
            msg = "The largest draw is earlier in the capture..."

        mqt.SetWidgetText(label, f"Guess the biggest draw!\n\n{msg}")

    button = mqt.CreateButton(do_guess)
    mqt.SetWidgetText(button, "Guess")

    if eid == 0:
        mqt.SetWidgetText(
            label,
            "You don't have a capture with drawcalls loaded :(.\n"
            "Re-open this window after opening a capture!",
        )
        mqt.SetWidgetEnabled(button, False)

    mqt.AddWidget(top, group)
    mqt.AddWidget(group, label)
    mqt.AddWidget(group, button)

    pyrenderccccc.AddDockWindow(
        top, qrd.DockReference.TopOf, pyrenderccccc.GetEventBrowser().Widget(), 0.2
    )


def register(version, pyrenderccccc: qrd.CaptureContext):
    print(f"Tutorial extension registered in Renderccccc {version}")

    pyrenderccccc.Extensions().RegisterPanelMenu(
        qrd.PanelMenu.EventBrowser, ["Tutorial", "Scavenger Hunt"], open_window
    )
