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

public class InfoDialogForDrawable extends InfoDialog
{
    private static final Component      STRUT = Box.createHorizontalStrut( 5 );
    private static final Component      GLUE  = Box.createHorizontalGlue();

    private static final String         FORMAT = Const.INFOBOX_TIME_FORMAT;
    private static       DecimalFormat  fmt = null;

    public InfoDialogForDrawable( final Frame     frame, 
                                  final double    clicked_time,
                                  final String[]  y_colnames,
                                  final Map       map_line2treenodes,
                                  final Drawable  dobj )
    {
        super( frame, "Drawable Info Box", clicked_time );

        /* Define DecialFormat for the displayed time */
        if ( fmt == null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( FORMAT );
        }
        
        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

            JPanel       top_panel = new JPanel();
            top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            CategoryLabel label_type = new CategoryLabel( dobj.getCategory() );
            top_panel.add( STRUT );
            top_panel.add( label_type );
            top_panel.add( GLUE );
        root_panel.add( top_panel );

            Primitive  prime  = (Primitive) dobj;

            Coord[]      coords = prime.getVertices();
            int          coords_length = coords.length;

            Coord        vertex;
            Integer      lineID;
            TreeNode[]   nodes;
            StringBuffer linebuf;
            StringBuffer textbuf = new StringBuffer();
            int          num_cols = 0, num_rows = 3;
            for ( int idx = 0; idx < coords_length; idx++ ) {
                linebuf = new StringBuffer( "[" + idx + "]: " );
                vertex  = coords[idx];
                lineID  = new Integer(vertex.lineID);
                nodes   = ( (YaxisTreeNode) map_line2treenodes.get( lineID ) )
                          .getPath();
                linebuf.append( "time = " + fmt.format(vertex.time) );
                for ( int ii = 1; ii < nodes.length; ii++ )
                    linebuf.append( ", " + y_colnames[ii-1]
                                  + " = " + nodes[ ii ] );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                linebuf.append( "\n" );
                textbuf.append( linebuf.toString() );
            }
            double duration = coords[coords_length-1].time - coords[0].time;
            textbuf.append( "duration = " + fmt.format(duration) );

            if ( prime instanceof Shadow ) {
                Shadow shade = (Shadow) prime;
                linebuf = new StringBuffer( "Number of Real Drawables = " );
                linebuf.append( shade.getNumOfRealObjects() );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                textbuf.append( "\n" + linebuf.toString() );
                num_rows++;
            }
            else {
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
        root_panel.add( new JScrollPane( text_area ) );

        root_panel.add( super.getCloseButton() );
    }
}
