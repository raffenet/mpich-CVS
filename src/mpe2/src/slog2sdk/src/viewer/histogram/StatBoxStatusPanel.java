/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import javax.swing.*;
import javax.swing.event.*;

import base.statistics.BufForTimeAveBoxes;
import viewer.common.LabeledTextField;

public class StatBoxStatusPanel extends JPanel
                                implements ChangeListener
{
    private BufForTimeAveBoxes      buf4statboxes;
    private LabeledTextField        fld_status;

    public StatBoxStatusPanel( final BufForTimeAveBoxes statboxes )
    {
        super();
        buf4statboxes  = statboxes;
        setLayout( new BoxLayout( this, BoxLayout.X_AXIS ) );

        fld_status   = new LabeledTextField( "   Statline   ", null );
        fld_status.setEditable( false );
        fld_status.setHorizontalAlignment( JTextField.CENTER );
        // fld_status.addActionListener( this );
        add( fld_status );

        super.setBorder( BorderFactory.createEtchedBorder() );
    }

    public void stateChanged( ChangeEvent evt )
    {
    }
    
}
