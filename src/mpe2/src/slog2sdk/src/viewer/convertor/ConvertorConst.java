/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.convertor;

import logformat.slog2.TraceName;

public class ConvertorConst
{
    public  static final String  CLOG_TO_SLOG2      = "  CLOG  -->  SLOG-2  ";
    public  static final String  RLOG_TO_SLOG2      = "  RLOG  -->  SLOG-2  ";
    public  static final String  UTE_TO_SLOG2       = "  UTE   -->  SLOG-2  ";

    public  static final String  CLOG_TO_SLOG2_JAR  = "clogTOslog2.jar";
    public  static final String  RLOG_TO_SLOG2_JAR  = "traceTOslog2.jar";
    public  static final String  UTE_TO_SLOG2_JAR   = "traceTOslog2.jar";

    public  static       char    File_Separator     = '/';

    public  static       String  CLOG_TraceDir      = "";
    public  static       String  RLOG_TraceDir      = "../trace_rlog/lib";
    public  static       String  UTE_TraceDir       = "";

    public  static String getDefaultConvertor( String filename )
    {
        String log_ext = TraceName.getLogFormatExtension( filename );
        if ( log_ext.equals( TraceName.CLOG_EXT ) )
            return CLOG_TO_SLOG2;
        else if ( log_ext.equals( TraceName.RLOG_EXT ) )
            return RLOG_TO_SLOG2;
        else if ( log_ext.equals( TraceName.UTE_EXT ) )
            return UTE_TO_SLOG2;
        else
            return "";
    }

    public  static String getDefaultSLOG2Name( String filename )
    {
        return TraceName.getDefaultSLOG2Name( filename );
    }

    public  static String getDefaultJarName( String convertor )
    {
        if ( convertor.equals( CLOG_TO_SLOG2 ) )
            return CLOG_TO_SLOG2_JAR;
        else if ( convertor.equals( RLOG_TO_SLOG2 ) )
            return RLOG_TO_SLOG2_JAR;
        else if ( convertor.equals( UTE_TO_SLOG2 ) )
            return UTE_TO_SLOG2_JAR;
        else
            return "";
    }

    public  static String getDefaultTraceDir( String convertor )
    {
        if ( convertor.equals( CLOG_TO_SLOG2 ) )
            return CLOG_TraceDir;
        else if ( convertor.equals( RLOG_TO_SLOG2 ) )
            return RLOG_TraceDir;
        else if ( convertor.equals( UTE_TO_SLOG2 ) )
            return UTE_TraceDir;
        else
            return "";
    }

    public  static void updateFileSeparatorOfTraceDirs( char new_file_sep )
    {
        if ( File_Separator != new_file_sep ) {
            if ( CLOG_TraceDir != null )
                CLOG_TraceDir.replace( File_Separator, new_file_sep );
            if ( RLOG_TraceDir != null )
                RLOG_TraceDir.replace( File_Separator, new_file_sep );
            if ( RLOG_TraceDir != null )
                RLOG_TraceDir.replace( File_Separator, new_file_sep );
            File_Separator = new_file_sep;
        }
    }
}
