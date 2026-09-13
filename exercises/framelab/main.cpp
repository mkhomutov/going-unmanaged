#include "FakeUi.h"
#include "panel.h"
#include <cstdio>

namespace {
int g_failures = 0;
void Check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        ++g_failures;
    }
}
}   // namespace

int main() {
    // 1. The host opens a document; the plug-in builds its panel under it.
    Node* root = FakeUi_OpenDocument();
    {
        Panel panel(root);
        Check(root->ChildCount() == 1, "the panel's frame hangs under the document root");
        Check(panel.Title() != nullptr && panel.Meter() != nullptr, "the panel's nodes exist");
        Check(FakeUi_LiveNodes() == 4, "root, frame, title, meter: four live nodes");

        // 2. A tooltip is shown (created unparented, handed to the frame)
        //    and hidden early (deleted by hand - the documented move).
        panel.ShowTooltip();
        Check(panel.Tooltip() != nullptr, "the tooltip exists while shown");
        Check(FakeUi_LiveNodes() == 5, "five live nodes with the tooltip up");
        panel.HideTooltip();
        Check(panel.Tooltip() == nullptr, "the framework's handle reads null once the tooltip is gone");
        Check(FakeUi_LiveNodes() == 4, "hiding the tooltip freed exactly one node");

        // 3. Shown again, and this time the HOST closes the document with
        //    the tooltip still up. The framework takes the whole tree.
        panel.ShowTooltip();
        FakeUi_CloseDocument();
        Check(FakeUi_LiveNodes() == 0, "closing the document freed every node, tooltip included");
        Check(!panel.Alive(), "the panel's handles read null after the close");
        Check(panel.Tooltip() == nullptr, "the tooltip's handle too");
        panel.HideTooltip();                 // must be a no-op now, not a second delete
    }   // ~Panel: nothing to delete, nothing double-freed

    // 4. A second document, and the panel torn down BEFORE the close - the
    //    other order, which must also leave the ledger at zero.
    root = FakeUi_OpenDocument();
    {
        Panel panel(root);
        panel.ShowTooltip();
    }
    Check(FakeUi_LiveNodes() == 5, "the panel's nodes outlive the Panel object: they are the root's");
    FakeUi_CloseDocument();
    Check(FakeUi_LiveNodes() == 0, "and the close frees them");

    if (g_failures != 0) {
        std::printf("framelab: %d FAILED\n", g_failures);
        return 1;
    }
    std::printf("framelab: live at unload %zu - the framework owned what it said it owned\n", FakeUi_LiveNodes());
    return 0;
}
