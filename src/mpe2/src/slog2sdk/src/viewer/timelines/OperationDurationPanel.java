/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;
import javax.swing.border.*;
import java.net.URL;

import base.drawable.TimeBoundingBox;
import base.statistics.BufForTimeAveBoxes;
import viewer.common.Const;

public class OperationDurationPanel extends JPanel
{
    private static final Component        GLUE  = Box.createHorizontalGlue();

    private static       Border           Normal_Border = null;
    private              JButton          stat_btn;

    private              TimeBoundingBox  timebox;
    private              CanvasTime       time_canvas;

    public OperationDurationPanel( final TimeBoundingBox  times,
                                   final CanvasTime       canvas )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        timebox      = times;
        time_canvas  = canvas;

        if ( Normal_Border == null ) {
            /*
            Normal_Border = BorderFactory.createCompoundBorder(
                            BorderFactory.createRaisedBevelBorder(),
                            BorderFactory.createLoweredBevelBorder() );
            */
            Normal_Border = BorderFactory.createEtchedBorder();
        }
        super.setBorder( Normal_Border );

        JPanel   stat_panel = new JPanel();
        JButton  stat_btn   = null;
        URL      icon_URL   = getURL( Const.IMG_PATH + "Stat110x40.gif" );
        if ( icon_URL != null )
            stat_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            stat_btn = new JButton( "Sumary Statistics" );
        stat_btn.setToolTipText(
        "Summary Statistics for the selected duration, timelines & legends" );
        stat_btn.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                BufForTimeAveBoxes  buf4statboxes =
                time_canvas.createBufForTimeAveBoxes( timebox );
                System.out.println( "Statistics = " + buf4statboxes );
            }
        } );
        stat_panel.add( GLUE );
        stat_panel.add( stat_btn );
        stat_panel.add( GLUE );
        super.add( stat_panel );
    }

    private URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }
}
