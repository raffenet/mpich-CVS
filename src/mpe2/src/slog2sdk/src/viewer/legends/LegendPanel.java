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

    private JButton      clear_btn;
    private JButton      close_btn;

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
 
        JPanel  end_panel = new JPanel();
        end_panel.setLayout( new BoxLayout( end_panel, BoxLayout.X_AXIS ) );
            end_panel.add( Box.createHorizontalGlue() );

            clear_btn = new JButton( "clear" );
            clear_btn.setToolTipText(
            "Clear all selections, i.e. display all" );
            // clear_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            clear_btn.addActionListener( this );
            end_panel.add( clear_btn );

            end_panel.add( Box.createHorizontalGlue() );

            close_btn = new JButton( "close" );
            close_btn.setToolTipText( "Hide this panel" );
            // close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            close_btn.addActionListener( this );
            end_panel.add( close_btn );

            end_panel.add( Box.createHorizontalGlue() );
        super.add( end_panel );
    }

    public void actionPerformed( ActionEvent evt )
    {
        Object evt_src = evt.getSource();
        if ( evt_src == close_btn )
            TopWindow.Legend.setVisible( false );
        else if ( evt_src == clear_btn )
            legend_list.clearSelection();
    }
}
