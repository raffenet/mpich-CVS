/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.legends;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;

import logformat.slog2.CategoryMap;
import logformat.slog2.input.InputLog;
import viewer.common.TopWindow;

public class LegendPanel extends JPanel
                         implements ActionListener
{
    private LegendList   legend_list;

    private JButton      hide_btn;

    public LegendPanel( final InputLog  slog_ins )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        Border  raised_border, lowered_border, empty_border, etched_border;
        raised_border  = BorderFactory.createRaisedBevelBorder();
        lowered_border = BorderFactory.createLoweredBevelBorder();
        empty_border   = BorderFactory.createEmptyBorder( 4, 4, 4 ,4 );
        etched_border  = BorderFactory.createEtchedBorder();

        legend_list  = new LegendList( slog_ins.getCategoryMap() );
        JScrollPane scroller = new JScrollPane( legend_list );
        scroller.setBorder( BorderFactory.createCompoundBorder(
                            lowered_border, BorderFactory.createCompoundBorder(
                                            empty_border, etched_border ) ) );
        super.add( scroller );
 
        hide_btn = new JButton( "hide" );
        hide_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
        hide_btn.addActionListener( this );
        super.add( hide_btn );
    }

    public void actionPerformed( ActionEvent evt )
    {
        if ( evt.getSource() == hide_btn )
            TopWindow.Legend.setVisible( false );
    }
}
