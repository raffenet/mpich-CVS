/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.launcher;

import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;
import java.util.StringTokenizer;

import viewer.common.Dialogs;
import viewer.common.RuntimeExecCommand;

public class Launcher
{
    // Assume Unix convention.
    private static       String  FileSeparator    = "/";
    private static       String  PathSeparator    = ":";
    private static       String  JavaHome         = null;
    private static       String  ClassPath        = null;
    private static       String  UserHome         = null;

    private static final String  SETUP_FILENAME   = ".jumpshot_launcher.conf";
    private static       String  JVM              = "java";
    private static       String  JVM_OPTIONS      = "-Xms64m -Xmx256m";
    private static       String  VIEWER_JAR       = "jumpshot.jar";

    private static void initializeSystemProperties()
    {
        Properties       sys_pptys;
        String           ppty_str;

        sys_pptys  = System.getProperties();

        FileSeparator  = sys_pptys.getProperty( "file.separator" );
        PathSeparator  = sys_pptys.getProperty( "path.separator" );
        JavaHome       = sys_pptys.getProperty( "java.home" );
        ClassPath      = sys_pptys.getProperty( "java.class.path" );
        UserHome       = sys_pptys.getProperty( "user.home" );
    }

    private static void initializeLauncherConstants()
    {
        if ( FileSeparator.equals( "/" ) )
            JVM = "java";
        else
            JVM = "javaw.exe";
    }

    private static final String CONFIGURATION_HEADER
                   = "# Jumpshot-4 Launcher setup file.\n"
                   + "#JVM: Java Virtual Machine name, can be absolute path.\n"
                   + "#JVM_OPTIONS: JVM launch parameters.\n"
                   + "#VIEWER_JAR: executable jar file to be launched.";

    private static boolean readLauncherConstants()
    {
        Properties   setup_pptys;
        String       setupfile_path;

        setupfile_path = UserHome + FileSeparator + SETUP_FILENAME;
        setup_pptys    = new Properties();
        try {
            FileInputStream fins = new FileInputStream( setupfile_path );
            setup_pptys.load( fins );
            fins.close();
            JVM          = setup_pptys.getProperty( "JVM" );
            JVM_OPTIONS  = setup_pptys.getProperty( "JVM_OPTIONS" );
            VIEWER_JAR   = setup_pptys.getProperty( "VIEWER_JAR" );
        } catch ( FileNotFoundException fioerr ) {
            return false;
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            System.exit( 1 );
        }
        return true;
    }

    private static void writeLauncherConstants()
    {
        Properties   setup_pptys;
        String       setupfile_path;

        setupfile_path = UserHome + FileSeparator + SETUP_FILENAME;
        setup_pptys    = new Properties();
        try {
            FileOutputStream fouts = new FileOutputStream( setupfile_path );
            setup_pptys.setProperty( "JVM", JVM );
            setup_pptys.setProperty( "JVM_OPTIONS", JVM_OPTIONS );
            setup_pptys.setProperty( "VIEWER_JAR", VIEWER_JAR );
            setup_pptys.store( fouts, CONFIGURATION_HEADER );
            fouts.close();
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            System.exit( 1 );
        }
    }

    private static String getDefaultPathToJVM()
    {
        String  path2jvm;
        File    jvm_file;

        path2jvm  = null;
        jvm_file  = new File( JVM );
        if ( jvm_file.isAbsolute() )
            path2jvm = JVM;
        else
            path2jvm = JavaHome + FileSeparator + "bin" + FileSeparator + JVM;
        return path2jvm;
    }

    private static String getDefaultPathToJarDir()
    {
        StringTokenizer  paths;
        String           path;
        String           path2jardir;
        int              char_idx;

        // System.out.println( "ClassPath = " + ClassPath );
        path2jardir  = null;
        paths        = new StringTokenizer( ClassPath, PathSeparator );
        while ( paths.hasMoreTokens() && path2jardir == null ) {
            path      = paths.nextToken();
            char_idx  = path.lastIndexOf( FileSeparator );
            if ( char_idx >= 0 )
                path2jardir = path.substring( 0, char_idx );
        }
        return path2jardir;
    }

    private static String getDefaultJarPath( String prefix )
    {
        if ( prefix != null && prefix.length() > 0 )
            return prefix + FileSeparator + VIEWER_JAR;
        else
            return VIEWER_JAR;
    }

    private String exec( String[] exec_cmd_ary )
    {
        Runtime            runtime;
        Process            proc;
        InputStreamThread  proc_err_task, proc_out_task;
        StringBuffer       proc_err_buf, proc_out_buf;
        int                proc_istatus;

        proc_err_buf  = new StringBuffer();
        proc_out_buf  = null;
        proc_istatus  = 0;
        runtime       = Runtime.getRuntime();
        try {
            proc = runtime.exec( exec_cmd_ary );
            proc_err_task = new InputStreamThread( proc.getErrorStream(),
                                                   "Error", proc_err_buf );
            proc_out_task = new InputStreamThread( proc.getInputStream(),
                                                   "Output", proc_out_buf );
            proc_err_task.start();
            proc_out_task.start();

            // Block THIS thread till process returns!
            proc_istatus = proc.waitFor();
            // Clean up InputStreamThread's when the proces is done.
            proc_err_task.stopRunning();
            proc_err_task = null;
            proc_out_task.stopRunning();
            proc_out_task = null;

            if ( proc_istatus != 0 )
                return proc_err_buf.toString();
        } catch ( Throwable err ) {
            err.printStackTrace();
        }
        return null;
    }

    
    public static final void main( String[] argv )
    {
        String              path2jardir;
        String              path2jvm;
        String              opt4jvm;
        String              jar_path;
        RuntimeExecCommand  exec_cmd;
        File                jar_file;
        File                jvm_file;
        Launcher            launcher;
        String              exec_err_msg;

        Launcher.initializeSystemProperties();
        Launcher.initializeLauncherConstants();
        if ( ! Launcher.readLauncherConstants() ) {
            Dialogs.info( null, "This is your first time using the launcher.\n"
                              + "A launcher setup file will be written to\n"
                              + "your home directory " + UserHome + ".", null );
            Launcher.writeLauncherConstants();
        }

        path2jardir = Launcher.getDefaultPathToJarDir();
        jar_path    = Launcher.getDefaultJarPath( path2jardir );
        jar_file    = new File( jar_path );
        if ( ! jar_file.exists() ) {
            Dialogs.error( null, jar_path + " does not exist!\n"
                               + "Make sure that " + VIEWER_JAR + " is in "
                               + "the same directory as this launcher.\n"
                               + "Then restart this program again." );
            System.exit( 1 );
        }
        if ( ! jar_file.canRead() ) {
            Dialogs.error( null, jar_path + " is NOT readable!\n"
                               + "Reset the read privilege of the file, "
                               + VIEWER_JAR + ".\n"
                               + "Then restart this program again." );
            System.exit( 1 );
        }

        path2jvm = Launcher.getDefaultPathToJVM();
        jvm_file = new File( path2jvm );
        if ( ! jvm_file.exists() ) {
            Dialogs.error( null, path2jvm + " does not exist!\n"
                               + "Make sure that " + path2jvm + " exists.\n"
                               + "Then restart this program again." );
            System.exit( 1 );
        }
        if ( ! jvm_file.canRead() ) {
            Dialogs.error( null, path2jvm + " is NOT readable!\n"
                               + "Reset the read privilege of the file, "
                               + path2jvm + ".\n"
                               + "Then restart this program again." );
            System.exit( 1 );
        }

        opt4jvm  = JVM_OPTIONS;
        exec_cmd = new RuntimeExecCommand();
        exec_cmd.addWholeString( path2jvm );
        exec_cmd.addTokenizedString( opt4jvm );
        exec_cmd.addWholeString( "-jar" );
        exec_cmd.addWholeString( jar_path );

        launcher = new Launcher();
        if (    ( exec_err_msg = launcher.exec( exec_cmd.toStringArray() ) )
             != null ) {
            Dialogs.error( null, "The following process exits with error:\n"
                               + exec_cmd.toString() + "\n" + exec_err_msg );
            System.exit( 1 );
        }

        // Preventive System.exit() to guarantee the launcher exits cleanly
        System.exit( 0 );
    }



    private class InputStreamThread extends Thread
    {
        private InputStream          ins;
        private String               prefix;
        private StringBuffer         outbuffer;
    
        private boolean              isRunning;
    
        public InputStreamThread( InputStream         the_ins,
                                  String              the_prefix,
                                  StringBuffer        the_outbuffer )
        {
            ins       = the_ins;
            prefix    = the_prefix;
            outbuffer = the_outbuffer;
    
            isRunning = true;
        }
    
        public void stopRunning()
        {
            isRunning = false;
        }
    
        public void run()
        {
            try {
                String  line = null;
                InputStreamReader ins_rdr = new InputStreamReader( ins );
                BufferedReader    buf_rdr = new BufferedReader( ins_rdr );
                while ( isRunning && ( line = buf_rdr.readLine() ) != null ) {
                    System.out.println( prefix + " > " + line );
                    if ( outbuffer != null )
                        outbuffer.append( line + "\n" );
                }
            } catch ( IOException ioerr ) {
                ioerr.printStackTrace();
            }
        }
    }

}
