/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import javax.swing.*;
import javax.swing.event.*;

import logformat.slog2.input.TreeTrunk;
import viewer.common.Const;
import viewer.common.LabeledTextField;

public class TreeTrunkPanel extends JPanel
                            implements ChangeListener
{
    private TreeTrunk               treetrunk;
    private LabeledTextField        fld_cur_depth;
    private LabeledTextField        fld_max_depth;

    public TreeTrunkPanel( TreeTrunk  in_treetrunk )
    {
        super();
        treetrunk  = in_treetrunk;
        setLayout( new BoxLayout( this, BoxLayout.X_AXIS ) );

        fld_cur_depth   = new LabeledTextField( "Lowest Depth",
                                                Const.SHORT_FORMAT );
        fld_cur_depth.setEditable( false );
        fld_cur_depth.setHorizontalAlignment( JTextField.CENTER );
        // fld_tree_depth.addActionListener( this );
        add( fld_cur_depth );

        fld_max_depth   = new LabeledTextField( "Max. Depth",
                                                Const.SHORT_FORMAT );
        fld_max_depth.setEditable( false );
        fld_max_depth.setHorizontalAlignment( JTextField.CENTER );
        add( fld_max_depth );

        super.setBorder( BorderFactory.createEtchedBorder() );
        fld_max_depth.setInteger(
                      treetrunk.getTreeRoot().getTreeNodeID().depth );
    }

    // public void lowestDepthChanged()
    public void stateChanged( ChangeEvent evt )
    {
        fld_cur_depth.setInteger( (int) treetrunk.getLowestDepth() );
    }
    
}
