/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.net.URL;
import javax.swing.ImageIcon;

public class Routines
{
    private static Cursor   normal_cursor       = null;
    private static Cursor   wait_cursor         = null;
    private static Cursor   hand_cursor         = null;
    private static Cursor   hand_open_cursor    = null;
    private static Cursor   hand_close_cursor   = null;
    private static Cursor   zoom_plus_cursor    = null;
    private static Cursor   zoom_minus_cursor   = null;

    private static Toolkit  toolkit             = null;

    static {
        ( new Routines() ).initCursors();
    }

    // private static URL getURL( String filename )
    private URL getURL( String filename )
    {
        // return ClassLoader.getSystemResource( Const.IMG_PATH + filename );
        return getClass().getResource( Const.IMG_PATH + filename );
    }

    private Image getBestCursorImage( String filename )
    {
        URL            icon_URL;
        Image          img;
        Dimension      opt_size;
        Graphics2D     g2d;
        int            iwidth, iheight;

        icon_URL = getURL( filename );
        img      = new ImageIcon( icon_URL ).getImage();
        iwidth   = img.getWidth( null );
        iheight  = img.getHeight( null );
        opt_size = toolkit.getBestCursorSize( iwidth, iheight );
        if ( opt_size.width == iwidth && opt_size.height == iheight )
            return img;
        else {
            BufferedImage  buf_img;
            buf_img = new BufferedImage( opt_size.width, opt_size.height,
                                          BufferedImage.TYPE_INT_ARGB );
            System.out.println( filename
                              + ": (" + iwidth + "," + iheight + ") -> ("
                              + opt_size.width + "," + opt_size.height + ")" );
            g2d     = buf_img.createGraphics();
            g2d.drawImage( img, 0, 0, null );
            g2d.dispose();
            return buf_img;
        }
    }
            
    public void initCursors()
    {
        normal_cursor  = Cursor.getPredefinedCursor( Cursor.DEFAULT_CURSOR );
        wait_cursor    = Cursor.getPredefinedCursor( Cursor.WAIT_CURSOR );
        hand_cursor    = Cursor.getPredefinedCursor( Cursor.HAND_CURSOR );

        Image    img;
        Point    pt;

        toolkit  = Toolkit.getDefaultToolkit();
        pt       = new Point( 1, 1 );

        img  = this.getBestCursorImage( "HandOpenUpLeft25.gif" ); 
        hand_open_cursor  = toolkit.createCustomCursor( img, pt,
                                                        "Hand Open" );
        img  = this.getBestCursorImage( "HandCloseUpLeft25.gif" );
        hand_close_cursor = toolkit.createCustomCursor( img, pt,
                                                        "Hand Close" );
        img  = this.getBestCursorImage( "ZoomPlusUpLeft25.gif" );
        zoom_plus_cursor  = toolkit.createCustomCursor( img, pt,
                                                        "Zoom Plus" );
        img  = this.getBestCursorImage( "ZoomMinusUpLeft25.gif" );
        zoom_minus_cursor = toolkit.createCustomCursor( img, pt,
                                                        "Zoom Minus" );
    }

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

    public static void setAllCursorsToHand( Component comp )
    {
        setAllCursors( comp, hand_cursor );
    }

    public static void setAllCursorsToHandOpen( Component comp )
    {
        setAllCursors( comp, hand_open_cursor );
    }

    public static void setAllCursorsToHandClose( Component comp )
    {
        setAllCursors( comp, hand_close_cursor );
    }

    public static void setAllCursorsToZoomPlus( Component comp )
    {
        setAllCursors( comp, zoom_plus_cursor );
    }

    public static void setAllCursorsToZoomMinus( Component comp )
    {
        setAllCursors( comp, zoom_minus_cursor );
    }

    public static boolean isCursorSetToZoomPlus( Component comp )
    {
        return comp.getCursor() == zoom_plus_cursor;
    }

    public static boolean isCursorSetToZoomMinus( Component comp )
    {
        return comp.getCursor() == zoom_minus_cursor;
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
