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
import javax.swing.tree.*;

import viewer.common.Dialogs;
import viewer.common.TopWindow;

public class ActionTimelineMark implements ActionListener
{
    private TimelineToolBar    toolbar;
    private YaxisTree          tree;
    private DefaultTreeModel   tree_model;

    public ActionTimelineMark( TimelineToolBar in_toolbar, YaxisTree in_tree )
    {
        toolbar      = in_toolbar;
        tree         = in_tree;
        tree_model   = (DefaultTreeModel) tree.getModel();
    }

    public void actionPerformed( ActionEvent event )
    {
        Debug.displayLine( "Action for Mark Timeline button" );
        TreePath[]        selected_paths;
        MutableTreeNode   node;

        if ( tree.getSelectionCount() < 1 ) {
            Dialogs.error( TopWindow.Timeline.getWindow(),
                           "At least ONE tree node needs to be marked!" );
            return;
        }

        selected_paths = tree.getSelectionPaths();
        for ( int idx = 0; idx < selected_paths.length; idx++ ) {
            node = (MutableTreeNode) selected_paths[idx].getLastPathComponent();
            if ( tree.isExpanded( selected_paths[ idx ] ) )
                Debug.displayLine( "\tselected an expanded node " + node );
            else
                Debug.displayLine( "\tselected a collapsed node " + node );
        }
        tree.renewCutAndPasteBuffer();
        if ( tree.isCutAndPasteBufferUniformlyLeveled( selected_paths ) )
            tree.addToCutAndPasteBuffer( selected_paths );
        else {
            Dialogs.error( TopWindow.Timeline.getWindow(),
                           "The tree nodes are NOT selected from the "
                         + "same level!  Select again." );
            return;
        }

        // Set toolbar buttons to reflect status
        toolbar.mark_btn.setEnabled( true );
        toolbar.move_btn.setEnabled( true );
        toolbar.delete_btn.setEnabled( true );
    }
}
