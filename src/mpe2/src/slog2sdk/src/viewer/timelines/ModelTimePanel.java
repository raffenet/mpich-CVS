/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;

import viewer.common.Const;
import viewer.common.LabeledTextField;

public class ModelTimePanel extends JPanel
                            implements TimeListener, ActionListener
{
    private ModelTime         model = null;

    private LabeledTextField  fld_tGlobal_min;
    private LabeledTextField  fld_tGlobal_max;
    private LabeledTextField  fld_tView_init;
    private LabeledTextField  fld_tView_final;
    private LabeledTextField  fld_tZoom_focus;
    private LabeledTextField  fld_iZoom_level;


    public ModelTimePanel( ModelTime model )
    {
        super();
        this.model        = model;
        setLayout( new BoxLayout( this, BoxLayout.X_AXIS ) );

        fld_iZoom_level   = new LabeledTextField( "Zoom Level",
                                                  Const.INTEGER_FORMAT );
        fld_iZoom_level.setEditable( false );
        fld_iZoom_level.setHorizontalAlignment( JTextField.CENTER );
        add( fld_iZoom_level ); // addSeparator();

        fld_tGlobal_min   = new LabeledTextField( "Global Min Time",
                                                  Const.PANEL_TIME_FORMAT );
        fld_tGlobal_min.setEditable( false );
        add( fld_tGlobal_min ); // addSeparator();

        fld_tView_init    = new LabeledTextField( "View  Init Time",
                                                  Const.PANEL_TIME_FORMAT );
        fld_tView_init.setEditable( false );
        add( fld_tView_init ); // addSeparator();

        fld_tZoom_focus   = new LabeledTextField( "Zoom Focus Time",
                                                  Const.PANEL_TIME_FORMAT );
        fld_tZoom_focus.setEditable( true );
        fld_tZoom_focus.addActionListener( this );
        add( fld_tZoom_focus );

        fld_tView_final   = new LabeledTextField( "View Final Time",
                                                  Const.PANEL_TIME_FORMAT );
        fld_tView_final.setEditable( false );
        add( fld_tView_final ); // addSeparator();

        fld_tGlobal_max   = new LabeledTextField( "Global Max Time",
                                                  Const.PANEL_TIME_FORMAT );
        fld_tGlobal_max.setEditable( false );
        add( fld_tGlobal_max ); // addSeparator();

        super.setBorder( BorderFactory.createEtchedBorder() );
    }

    public void zoomLevelChanged()
    {
        fld_iZoom_level.setInteger( model.getZoomLevel() );
    }

    /*
        timeChanged() is invoked by ModelTime's updateParamDisplay()
    */
    public void timeChanged( TimeEvent evt )
    {
        if ( Debug.isActive() )
            Debug.println( "ModelTimePanel: timeChanged()'s START: " );
        fld_tGlobal_min.setDouble( model.getTimeGlobalMinimum() );
        fld_tView_init.setDouble( model.getTimeViewPosition() );
        fld_tZoom_focus.setDouble( model.getTimeZoomFocus() );
        fld_tView_final.setDouble( model.getTimeViewPosition()
                                 + model.getTimeViewExtent() );
        fld_tGlobal_max.setDouble( model.getTimeGlobalMaximum() );
        if ( Debug.isActive() )
            Debug.println( "ModelTimePanel: timeChanged()'s END: " );
    }

    public void actionPerformed( ActionEvent evt )
    {
        // if ( evt.getSource() == fld_tZoom_focus )
            model.setTimeZoomFocus( fld_tZoom_focus.getDouble() );
    }
}
