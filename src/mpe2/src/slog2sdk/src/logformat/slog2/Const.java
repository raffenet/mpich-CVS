/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

public class Const
{
           static final String  version_ID     = "SLOG 2.0.2";
           static final byte    INVALID_byte   = Byte.MIN_VALUE;
           static final short   INVALID_short  = Short.MIN_VALUE;
           static final int     INVALID_int    = Integer.MIN_VALUE;
           static final long    INVALID_long   = Long.MIN_VALUE;
           static final int     NULL_int       = 0;
           static final int     NULL_iaddr     = 0;
           static final long    NULL_fptr      = 0;
           static final float   INVALID_float  = Float.MIN_VALUE;
           static final double  INVALID_double = Double.MIN_VALUE;
           static final int     TRUE           = 1;
           static final int     FALSE          = 0;

    public static final short   NUM_LEAFS      = 2;
    public static final int     LEAF_BYTESIZE  = 65536;

    public static final String  VERSION_HISTORY =
                                  "Version SLOG 2.0.0's node employs "
                                + "decreasing endtime ordering.\n"
                                + "Version SLOG 2.0.1's node employs "
                                + "increasing starttime ordering.\n"
                                + "Version SLOG 2.0.2 added preview "
                                + "data to shadow states.\n"
                                + "Using 2.0.1 viewer for 2.0.0 logfile "
                                + "messes up state nesting stack!\n"
                                + "2.0.2 viewer cannot read 2.0.1 logfile\n";
}
