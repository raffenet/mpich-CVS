/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.Font;

public class Const
{
    public  static final int     FONT_SIZE = 10;
    public  static final Font    FONT      = new Font( "SansSerif",
                                                       Font.PLAIN, FONT_SIZE );

    public  static final int     MIN_ZOOM_LEVEL         = 0;
    public  static final int     MAX_ZOOM_LEVEL         = 30;

    public  static final String  PANEL_TIME_FORMAT      = "#,##0.00########";
    public  static final String  RULER_TIME_FORMAT      = "#,##0.00######";
    public  static final String  INTEGER_FORMAT         = "#,##0";
    public  static final String  FLOAT_FORMAT           = "0.0##";
    public  static final String  SHORT_FORMAT           = "##0";
    public  static final String  STRING_FORMAT          = null;;
    public  static final String  BOOLEAN_FORMAT         = null;;

    public  static final int     TIME_SCROLLBAR_UNIT_INCREMENT = 20;
}
