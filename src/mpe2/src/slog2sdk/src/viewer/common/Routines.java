/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.*;

public class Routines
{
    private static Cursor normal_cursor = new Cursor( Cursor.DEFAULT_CURSOR );
    private static Cursor wait_cursor   = new Cursor( Cursor.WAIT_CURSOR );

    public static void setAllCursors( Component comp, Cursor csr )
    {
        if ( comp == null ) return;
        comp.setCursor( csr );
        if ( comp instanceof Container ) {
            Component [] comps = ( (Container) comp ).getComponents();
            for ( int ii = 0; ii < comps.length; ii++)
                setAllCursors( comps[ ii ], csr );
        }
    }

    public static void setAllCursorsToNormal( Component comp )
    {
        setAllCursors( comp, normal_cursor );
    }

    public static void setAllCursorsToWait( Component comp )
    {
        setAllCursors( comp, wait_cursor );
    }

    public static void setCursorToNormal( Component comp )
    {
        if ( comp == null ) return;
        comp.setCursor( normal_cursor );
    }

    public static void setCursorToWait( Component comp )
    {
        if ( comp == null ) return;
        comp.setCursor( wait_cursor );
    }

    public static Dimension getScreenSize()
    {
        return Toolkit.getDefaultToolkit().getScreenSize();
    }

    public static Dimension correctSize( Dimension size, final Insets insets )
    {
        if ( insets != null ) {
            size.width  += insets.left + insets.right;
            if ( size.width > Short.MAX_VALUE )
                size.width  = Short.MAX_VALUE;
            size.height += insets.top + insets.bottom;
            if ( size.height > Short.MAX_VALUE )
                size.height = Short.MAX_VALUE;
        }
        return size;
    }

    //  JTextField.getColumnWidth() uses char('m') defines column width
    //  getAdjNumOfTextColumns() computes the effective char column number
    //  that is needed by the JTextField's setColumns().
    //  This routine should be good for both JTextField and JTextArea
    public static int getAdjNumOfTextColumns( Component textcomp,
                                              int num_numeric_columns )
    {
        FontMetrics metrics;
        int         num_char_columns;

        metrics = textcomp.getFontMetrics( textcomp.getFont() );
        num_char_columns = (int) Math.ceil( (double) num_numeric_columns
                                          * metrics.charWidth( '1' )
                                          / metrics.charWidth( 'm' ) );
        // System.out.println( "num_char_columns = " + num_char_columns );
        return num_char_columns;
    }
}
