/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;
import java.util.Map;
import java.util.Iterator;

import base.drawable.CategoryWeight;
import base.topology.SummaryState;
import viewer.common.Const;
import viewer.common.Routines;
import viewer.common.Parameters;
import viewer.legends.CategoryLabel;
import viewer.zoomable.TimeFormat;


public class InfoPanelForCategory extends JPanel
{
    private static final Component      STRUT = Box.createHorizontalStrut( 10 );
    private static final Component      GLUE  = Box.createHorizontalGlue();

    private static final String         FORMAT = Const.INFOBOX_TIME_FORMAT;
    private static       DecimalFormat  fmt    = null;
    private static       TimeFormat     tfmt   = null;

    private static       Border         Normal_Border = null;

    private              CategoryWeight category_weight;



    public InfoPanelForCategory( final String[]        y_colnames,
                                 final TreePath        ylabel_path,
                                 final CategoryWeight  twgt )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        /* Define DecialFormat for the displayed time */
        if ( fmt == null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( FORMAT );
        }
        if ( tfmt == null )
            tfmt = new TimeFormat();
        if ( Normal_Border == null ) {
            /*
            Normal_Border = BorderFactory.createCompoundBorder(
                            BorderFactory.createRaisedBevelBorder(),
                            BorderFactory.createLoweredBevelBorder() );
            */
            Normal_Border = BorderFactory.createEtchedBorder();
        }

        category_weight  = twgt;

        // Set the CategoryLabel Icon
        Dimension     panel_max_size;
        CategoryLabel label_type = null;
        JPanel        top_panel  = new JPanel();
        top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            label_type = new CategoryLabel( twgt.getCategory() );
            top_panel.add( STRUT );
            top_panel.add( label_type );
            top_panel.add( GLUE );
            top_panel.setBorder( Normal_Border );
        top_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        panel_max_size        = top_panel.getPreferredSize();
        panel_max_size.width  = Short.MAX_VALUE;
        top_panel.setMaximumSize( panel_max_size );
        super.add( top_panel );

        // Determine the text of the drawable
        TextAreaBuffer  textbuf;
        int             num_cols, num_rows;
        textbuf = new TextAreaBuffer( y_colnames, ylabel_path );
            textbuf.setCategoryWeightText( category_weight );
        textbuf.finalized();
        num_cols  = textbuf.getColumnCount();
        num_rows  = textbuf.getRowCount();

        // Set the TextArea
        JTextArea  text_area;
        int        adj_num_cols;
        text_area    = new JTextArea( textbuf.toString() );
        adj_num_cols = Routines.getAdjNumOfTextColumns( text_area, num_cols );
        num_cols     = (int) Math.ceil( adj_num_cols * 85.0d / 100.0d );
        text_area.setColumns( num_cols );
        text_area.setRows( num_rows );
        text_area.setEditable( false );
        text_area.setLineWrap( true );
        JScrollPane scroller = new JScrollPane( text_area );
        scroller.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( scroller );
    }

    public CategoryWeight getCategoryWeight()
    {
        return category_weight;
    }



    private class TextAreaBuffer
    {
        private              String[]       y_colnames;
        private              Object[]       y_labels;
        private              StringBuffer   strbuf;
        private              String         strbuf2str;
        private              int            num_cols;
        private              int            num_rows;

        public TextAreaBuffer( final String[]  in_y_colnames,
                               final TreePath  in_ylabel_path )
        {
            y_colnames          = in_y_colnames;
            y_labels            = in_ylabel_path.getPath();
            strbuf              = new StringBuffer();
            strbuf2str          = null;

            // Initialize num_cols and num_rows.
            num_cols            = 0;
            num_rows            = 0;
        }

        // this.finalized() needs to be called before
        // getColumnCount()/getRowCount()/toString()
        public void finalized()
        {
            int num_lines;
            strbuf2str = strbuf.toString();
            num_lines  = this.getNumOfLines();
            num_rows   = num_lines;
        }

        public int getColumnCount()
        { return num_cols; }

        public int getRowCount()
        { return num_rows; }

        public String toString()
        { return strbuf2str; }

        private int getNumOfLines()
        {
            int num_lines;
            int str_length;
            int ipos;
            if ( strbuf2str != null ) {
                num_lines  = 1;
                ipos       = 0;
                str_length = strbuf2str.length();
                while ( ipos >= 0 && ipos < str_length ) {
                    ipos = strbuf2str.indexOf( '\n', ipos );
                    if ( ipos >= 0 ) {
                        num_lines++;
                        ipos++;
                    }
                }
                return num_lines;
            }
            else
                return -1;
        }

        public void setCategoryWeightText( final CategoryWeight  twgt )
        {
            String            twgt_str;
            StringBuffer      linebuf;
            int               idx;
            int               print_status;
            if ( SummaryState.isDisplayTypeEqualWeighted() ) {
                print_status = CategoryWeight.PRINT_ALL_RATIOS;
                strbuf.append( "*** All Duration Ratio:" );
            }
            else {
                if ( SummaryState.isDisplayTypeExclusiveRatio() ) {
                    print_status = CategoryWeight.PRINT_EXCL_RATIO;
                    strbuf.append( "*** Exclusive Duration Ratio:" );
                }
                else {
                    strbuf.append( "*** Inclusive Duration Ratio:" );
                    print_status = CategoryWeight.PRINT_INCL_RATIO;
                }
            }
            twgt_str = twgt.toInfoBoxString( print_status );
            if ( num_cols < twgt_str.length() )
                num_cols = twgt_str.length();
            num_rows++;
            strbuf.append( "\n" + twgt_str );

            linebuf = new StringBuffer( "At" );
            for ( idx = 1; idx < y_labels.length; idx++ )
                linebuf.append( " " + y_colnames[ idx-1 ]
                              + "=" + y_labels[ idx ] );
            if ( num_cols < linebuf.length() )
                num_cols = linebuf.length();
            num_rows++;
            strbuf.append( "\n" + linebuf.toString() );
        }
    
    }   //  End of   private class TextAreaBuffer
}
