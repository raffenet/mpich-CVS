/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.*;
import java.awt.event.*;
import java.net.*;
import javax.swing.*;

public class ActionYaxisTreeCollapse implements ActionListener
{
    private TimelineToolBar  toolbar;
    private YaxisTree        tree_view;

    public ActionYaxisTreeCollapse( TimelineToolBar in_toolbar,
                                    YaxisTree       in_tree )
    {
        toolbar    = in_toolbar;
        tree_view  = in_tree;
    }

    public void actionPerformed( ActionEvent event )
    {
        Debug.displayLine( "Action for Collapse Tree button" );

        tree_view.collapseLevel();
        toolbar.commit_btn.doClick();

        // Set toolbar buttons to reflect status
        toolbar.expand_btn.setEnabled( tree_view.isLevelExpandable() );
        toolbar.collapse_btn.setEnabled( tree_view.isLevelCollapsable() );
        toolbar.commit_btn.setEnabled( true );
    }
}
