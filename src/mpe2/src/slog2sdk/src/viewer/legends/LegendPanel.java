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
    private LegendTable  legend_table;

    private JButton      visible_btn;
    private JButton      search_btn;
    private JButton      all_btn;
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

        legend_table  = new LegendTable( slog_ins.getCategoryMap() );
        JScrollPane scroller = new JScrollPane( legend_table,
                                   JScrollPane.VERTICAL_SCROLLBAR_ALWAYS,
                                   JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS );
        scroller.setBorder( BorderFactory.createCompoundBorder(
                            lowered_border, BorderFactory.createCompoundBorder(
                                            empty_border, etched_border ) ) );
        super.add( scroller );
 

        Border titled_border;

        JPanel  selected_panel = new JPanel();
        selected_panel.setLayout( new BoxLayout( selected_panel,
                                               BoxLayout.X_AXIS ) );
            selected_panel.add( Box.createHorizontalGlue() );

            visible_btn = new JButton( "Visible" );
            visible_btn.setToolTipText(
                        "Toggle all selected Visiblility checkboxes" );
            // visible_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            visible_btn.addActionListener( this );
            selected_panel.add( visible_btn );

            selected_panel.add( Box.createHorizontalGlue() );

            search_btn = new JButton( "Searchable" );
            search_btn.setToolTipText(
                        "Toggle all selected Searchability checkboxes" );
            // search_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            search_btn.addActionListener( this );
            selected_panel.add( search_btn );

            selected_panel.add( Box.createHorizontalGlue() );
        titled_border = BorderFactory.createTitledBorder(
                                      etched_border, " Selected Toggles " );
        selected_panel.setBorder( titled_border );
        super.add( selected_panel );

        JPanel  all_panel = new JPanel();
        all_panel.setLayout( new BoxLayout( all_panel,
                                               BoxLayout.X_AXIS ) );
            all_panel.add( Box.createHorizontalGlue() );

            all_btn = new JButton( "Select" );
            all_btn.setToolTipText( "Select all Legends" );
            // all_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            all_btn.addActionListener( this );
            all_panel.add( all_btn );

            all_panel.add( Box.createHorizontalGlue() );

            clear_btn = new JButton( "Deselect" );
            clear_btn.setToolTipText( "Deselect all Legends" );
            // clear_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            clear_btn.addActionListener( this );
            all_panel.add( clear_btn );

            all_panel.add( Box.createHorizontalGlue() );
        titled_border = BorderFactory.createTitledBorder(
                                      etched_border, " All Toggles " );
        all_panel.setBorder( titled_border );
        super.add( all_panel );

        JPanel  end_panel = new JPanel();
        end_panel.setLayout( new BoxLayout( end_panel, BoxLayout.X_AXIS ) );
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
        else if ( evt_src == visible_btn )
            legend_table.toggleVisibilityCheckboxes();
        else if ( evt_src == search_btn )
            legend_table.toggleSearchabilityCheckboxes();
        else if ( evt_src == all_btn )
            legend_table.selectAll();
        else if ( evt_src == clear_btn )
            legend_table.clearSelection();
    }
}
