/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.io.*;
import javax.swing.*;

public class Debug
{
    private static boolean       msg_on           = false;
    private static String        filename         = null;
    private static int           ilevel           = 0;
    private static final String  unit_ident_str   = "    ";
    private static boolean       isLineHead       = true;

    private static JTextArea     text_area        = null;

    public static void initTextArea()
    {
        /*
          Bug in JTextArea ?  without specified Nrow and Ncolumn,
          the vertical scrollbar does NOT show up.  For example,
          the following does NOT work.

          text_area = new JTextArea();
          text_area.setPreferredSize( new Dimension( 256,128 ) );
        */
                            text_area = new JTextArea( 10, 30 );
            JScrollPane text_scroller = new JScrollPane( text_area );

        JFrame text_frame = new JFrame( "Debugging Output" );
        text_frame.getContentPane().add( text_scroller );
        text_frame.pack();
        text_frame.setVisible( true );
    
        setMessageOn();
    }

    public static void setMessageOn() { msg_on = true; }
    public static void setMessageOff() { msg_on = false; }

    public static void setFilename( String in_name )
    throws IOException
    {
        filename = new String( in_name );
        System.setOut( new PrintStream( new FileOutputStream(in_name) ) );
    }

    public static void print( String str )
    {
        if ( msg_on ) {
            if ( str.indexOf( "END" ) > 0 )
                ilevel -= 1;

            if ( isLineHead )
                for ( int ii = ilevel; ii > 0; ii -= 1 )
                    System.out.print( unit_ident_str );
            System.out.print( str );
            isLineHead = false;

            if ( str.indexOf( "START" ) > 0 )
                ilevel += 1;
        }
    }

    public static void println( String str )
    {
        if ( msg_on ) {
            if ( str.indexOf( "END" ) > 0 )
                ilevel -= 1;

            if ( isLineHead )
                for ( int ii = ilevel; ii > 0; ii -= 1 )
                   System.out.print( unit_ident_str );
            System.out.println( str );
            isLineHead = true;

            if ( str.indexOf( "START" ) > 0 )
                ilevel += 1;

            if ( ilevel == 0 )
                System.out.flush();
        }
    }

    public static void displayLine( String txt_str )
    {
        if ( msg_on ) {
            text_area.append( txt_str + "\n" );
            text_area.setCaretPosition( text_area.getDocument().getLength() );
        }
    }
}
