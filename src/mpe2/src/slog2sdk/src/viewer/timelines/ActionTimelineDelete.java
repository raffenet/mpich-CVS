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

public class ActionTimelineDelete implements ActionListener
{
    private TimelineToolBar    toolbar;
    private YaxisTree          tree;
    private DefaultTreeModel   tree_model;

    public ActionTimelineDelete( TimelineToolBar in_toolbar, YaxisTree in_tree )
    {
        toolbar      = in_toolbar;
        tree         = in_tree;
        tree_model   = (DefaultTreeModel) tree.getModel();
    }

    public void actionPerformed( ActionEvent event )
    {
        Debug.displayLine( "Action for Delete Timeline button" );
        TreePath[]        child_paths;
        MutableTreeNode   child;
        int               idx;
        
        if ( ! Dialogs.confirm( TopWindow.Timeline.getWindow(),
               "Are you sure to PERMANENTLY delete the marked items?" ) )
            return;

        child_paths    = tree.getFromCutAndPasteBuffer();

        if ( child_paths.length < 1 ) {
            Dialogs.warn( TopWindow.Timeline.getWindow(),
                          "Nothing has been marked for removal!" );
            return;
        }

        // Collapse any marked timelines which are expanded
        for ( idx = 0; idx < child_paths.length; idx++ ) {
            if ( tree.isExpanded( child_paths[ idx ] ) ) {
                Debug.displayLine( "\tCollapse " + child_paths[ idx ] );
                tree.collapsePath( child_paths[ idx ] );
            }
        }

        // Remove the marked timelines from the tree
        for ( idx = 0; idx < child_paths.length; idx++ ) {
            child = (MutableTreeNode) child_paths[ idx ].getLastPathComponent();
            tree_model.removeNodeFromParent( child );
            Debug.displayLine( "\tCut " + child );
        }

        // Clear up the marked timelines in the buffer
        tree.clearCutAndPasteBuffer(); 

        // Update leveled_paths[]
        tree.update_leveled_paths();

        // Set toolbar buttons to reflect status
        toolbar.mark_btn.setEnabled( true );
        toolbar.move_btn.setEnabled( false );
        toolbar.delete_btn.setEnabled( false );
        toolbar.expand_btn.setEnabled( tree.isLevelExpandable() );
        toolbar.collapse_btn.setEnabled( tree.isLevelCollapsable() );
        toolbar.commit_btn.setEnabled( true );
    }
}
