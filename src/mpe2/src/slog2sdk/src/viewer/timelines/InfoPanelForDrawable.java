/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.*;
import javax.swing.*;
import javax.swing.tree.TreeNode;
import java.util.Map;

import base.drawable.Coord;
import base.drawable.Drawable;
import base.drawable.Primitive;
import base.drawable.Shadow;
import viewer.common.Const;
import viewer.common.Routines;
import viewer.legends.CategoryLabel;

public class InfoPanelForDrawable extends JPanel
{
    private static final Component      STRUT = Box.createHorizontalStrut( 5 );
    private static final Component      GLUE  = Box.createHorizontalGlue();

    private static final String         FORMAT = Const.INFOBOX_TIME_FORMAT;
    private static       DecimalFormat  fmt = null;

    public InfoPanelForDrawable( final Map       map_line2treenodes,
                                 final String[]  y_colnames,
                                 final Drawable  dobj )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        /* Define DecialFormat for the displayed time */
        if ( fmt == null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( FORMAT );
        }

            JPanel       top_panel = new JPanel();
            top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            CategoryLabel label_type = new CategoryLabel( dobj.getCategory() );
            top_panel.add( STRUT );
            top_panel.add( label_type );
            top_panel.add( GLUE );
        super.add( top_panel );

            Primitive  prime  = (Primitive) dobj;

            Coord[]        coords = prime.getVertices();
            int            coords_length = coords.length;

            Coord          vertex;
            Integer        lineID;
            YaxisTreeNode  node;
            TreeNode[]     nodes;
            StringBuffer   linebuf;
            StringBuffer   textbuf = new StringBuffer();
            int            num_cols, num_rows;
            double         duration;
            int            idx, ii;

            if ( prime instanceof Shadow ) {
                num_cols = 0;
                num_rows = 3;
                Shadow shade = (Shadow) prime;
                duration = shade.getLatestTime() - shade.getEarliestTime();
                linebuf = new StringBuffer();
                linebuf.append( "duration = (max)" + fmt.format(duration) );
                duration = coords[coords_length-1].time - coords[0].time;
                linebuf.append( ", (ave)" + fmt.format(duration) );
                num_cols = linebuf.length();
                textbuf.append( linebuf.toString() );
                    idx     = 0;
                    textbuf.append( "\n" );
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = (min)"
                                  + fmt.format(shade.getEarliestTime())
                                  + ", (ave)" + fmt.format(vertex.time) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( linebuf.toString() );

                    idx     = coords_length-1;
                    textbuf.append( "\n" );
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = (max)"
                                  + fmt.format(shade.getLatestTime())
                                  + ", (ave)" + fmt.format(vertex.time) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( linebuf.toString() );

                linebuf = new StringBuffer( "Number of Real Drawables = " );
                linebuf.append( shade.getNumOfRealObjects() );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                textbuf.append( "\n" + linebuf.toString() );
                num_rows++;
            }
            else {
                num_cols = 0;
                num_rows = 3;
                duration = coords[coords_length-1].time - coords[0].time;
                linebuf = new StringBuffer();
                linebuf.append( "duration = " + fmt.format(duration) );
                num_cols = linebuf.length();
                textbuf.append( linebuf.toString() );
                for ( idx = 0; idx < coords_length; idx++ ) {
                    textbuf.append( "\n" );
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = " + fmt.format(vertex.time) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( linebuf.toString() );
                }
                
                String info_str = prime.toInfoBoxString().trim();
                if ( info_str.length() > 0 ) {
                    textbuf.append( "\n" + info_str );
                    num_rows++;
                }
            }

            JTextArea text_area = new JTextArea( textbuf.toString() );
            int adj_num_cols    = Routines.getAdjNumOfTextColumns( text_area,
                                                                   num_cols );
            text_area.setColumns( adj_num_cols );
            text_area.setRows( num_rows );
            text_area.setEditable( false );
            text_area.setLineWrap( true );
        super.add( new JScrollPane( text_area ) );
    }
}
