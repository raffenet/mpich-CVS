/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.Dimension;
import java.awt.Rectangle;
import javax.swing.JFrame;

public abstract class TopWindow
{
    private JFrame  root;

    public TopWindow()
    { root = null; }

    public void setWindow( JFrame in_root )
    { root = in_root; }

    public JFrame getWindow()
    { return root; }

    public void setVisible( boolean isVisible )
    {
        if ( root != null )
            root.setVisible( isVisible );
    }

    public void dispose()
    {
        if ( root != null ) {
            root.dispose();
            root = null;
        }
    }

    public abstract void disposeAll(); 

    public static void layoutIdealLocations()
    {
        if ( ! Parameters.AUTO_WINDOWS_LOCATION )
            return;

        Rectangle bounds = new Rectangle();
        JFrame legend_frame   = Legend.getWindow();
        if ( legend_frame != null ) {
            legend_frame.setLocation( bounds.getLocation() );
            bounds = legend_frame.getBounds();
        }
        JFrame first_frame    = First.getWindow();
        if ( first_frame != null ) {
            bounds.x += bounds.width;
            first_frame.setLocation( bounds.getLocation() );
            bounds = first_frame.getBounds();
        }
        JFrame timeline_frame = Timeline.getWindow();
        if ( timeline_frame != null ) {
            if ( first_frame != null )
                bounds.y += bounds.height;
            else
                bounds.x += bounds.width;
            timeline_frame.setLocation( bounds.getLocation() );
            bounds = timeline_frame.getBounds();
        }
        JFrame prefer_frame   = Preference.getWindow();
        if ( prefer_frame != null ) {
            bounds.x += bounds.width;
            prefer_frame.setLocation( bounds.getLocation() );
        }
    }



    public static TopWindow  Preference  = new TopWindow()
    {
        public void disposeAll()
        {
            dispose();
        }
    };


    public static TopWindow  Timeline    = new TopWindow()
    {
        public void disposeAll()
        {
            dispose();
        }
    };


    public static TopWindow  Legend      = new TopWindow()
    {
        public void disposeAll()
        {
            Timeline.disposeAll();
            dispose();
        }
    };


    public static TopWindow  First       = new TopWindow()
    {
        public void disposeAll()
        {
            Legend.disposeAll();
            if ( Dialogs.confirm( First.getWindow(),
                 "Are you sure you want to exit Jumpshot-4 ?" ) ) {
                Preference.disposeAll();
                dispose();
                System.exit( 0 );
            }
        }
    };
}
