/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class ViewportTime extends JViewport
                          implements TimeListener,
                                     ComponentListener
//                                     HierarchyBoundsListener
{
    private Point                     view_pt;
    // view_img is both a Component and ScrollableImage object
    private ScrollableImage           view_img    = null;

    public ViewportTime()
    {
        view_pt = new Point( 0, 0 );
        /*
            For resizing of the viewport => resizing of the ScrollableImage
        */
        addComponentListener( this );
        /*
            HierarchyBoundsListener is for the case when this class
            is moved but NOT resized.  That it checks for situation
            to reinitialize the size of ScrollableImage when the 
            scrollable image's size is reset for some obscure reason.

            However, defining getPreferredSize() of ScrollableImage
            seems to make HierarchyBoundsListener of this class
            unnecessary.
        */
        // addHierarchyBoundsListener( this );

        // setDebugGraphicsOptions( DebugGraphics.LOG_OPTION );
    }

    public void setView( Component view )
    {
        super.setView( view );
        // Assume "view" has implemented the ComponentListener interface
        Dimension min_sz = view.getMinimumSize();
        if ( min_sz != null )
            setMinimumSize( min_sz );
        Dimension max_sz = view.getMaximumSize();
        if ( max_sz != null )
            setMaximumSize( max_sz );
        Dimension pref_sz = view.getPreferredSize();
        if ( pref_sz != null )
            setPreferredSize( pref_sz );
        view_img = (ScrollableImage) view;
    }

    //  For Debugging Profiling
    public Dimension getMinimumSize()
    {
        Dimension min_sz = super.getMinimumSize();
        Debug.println( "ViewportTime: min_size = " + min_sz );
        return min_sz;
    }

    //  For Debugging Profiling
    public Dimension getMaximumSize()
    {
        Dimension max_sz = super.getMaximumSize();
        Debug.println( "ViewportTime: max_size = " + max_sz );
        return max_sz;
    }

    //  For Debugging Profiling
    public Dimension getPreferredSize()
    {
        Dimension pref_sz = super.getPreferredSize();
        Debug.println( "ViewportTime: pref_size = " + pref_sz );
        return pref_sz;
    }

    protected void setYaxisViewPosition( int new_y_view_pos )
    {
        view_pt.y   = new_y_view_pos;
    }

    protected int  getXaxisViewPosition()
    {
        return view_pt.x;
    }

    /*
        timeChanged() is invoked by ModelTime's fireTimeChanged();

        Since ModelTime is the Model for the scroll_bar, timeChanged()
        will be called everytime when scroll_bar is moved/changed. 
    */
    public void timeChanged( TimeEvent evt )
    {
        Debug.println( "ViewportTime: timeChanged()'s START: " );
        Debug.println( "time_evt = " + evt );
        if ( view_img != null ) {
            // view_img.checkToXXXXView() assumes constant image size
            view_img.checkToZoomView();
            view_img.checkToScrollView();
            Debug.println( "ViewportTime:timeChanged()'s view_img = "
                         + view_img );
            view_pt.x = view_img.getXaxisViewPosition();
            super.setViewPosition( view_pt );
            /*
               calling view.repaint() to ensure the view is repainted
               after setViewPosition is called.
               -- apparently, super.repaint(), the RepaintManager, has invoked 
                  ( (Component) view_img ).repaint();
               -- JViewport.setViewPosition() may have invoked super.repaint()
            */
            super.repaint();

            Debug.println( "ViewportTime: view_img.getXaxisViewPosition() = "
                         + view_pt.x );
            Debug.println( "ViewportTime: [after] getViewPosition() = "
                         + super.getViewPosition() );
        }
        Debug.println( "ViewportTime: timeChanged()'s END: " );
    }

    public void componentResized( ComponentEvent evt )
    {
        Debug.println( "ViewportTime: componentResized()'s START: " );
        Debug.println( "comp_evt = " + evt );
        if ( view_img != null ) {
            /*
               Instead of informing the view by ComponentEvent, i.e.
               doing addComponentListener( (ComponentListener) view ),
               ( (ComponentListener) view ).componentResized() is called
               directly here to ensure that view is resized before 
               super.setViewPosition() is called on view.  This is done
               to ensure the correct sequence of componentResized().
               This also means the "view" does NOT need to implement
               ComponentListener interface.
            */
            view_img.componentResized( this );
            /*
               It is very IMPORTANT to do setPreferredSize() for JViewport
               with custom JComponent view.  If PreferredSize is NOT set,
               the top-level container, JFrame, will have difficulty to
               compute the size final display window when calling
               Window.pack().  The consequence will be the initial
               view of JViewport has its getViewPosition() set to (0,0)
               in view coordinates during program starts up.
               Apparently, Window.pack() uses PreferredSize to compute
               window size.
            */
            this.setPreferredSize( getSize() );
            Debug.println( "ViewportTime: componentResized()'s view_img = "
                         + view_img );
            view_pt.x = view_img.getXaxisViewPosition();
            super.setViewPosition( view_pt );
            /*
               calling view.repaint() to ensure the view is repainted
               after setViewPosition is called.
               -- apparently, this.repaint(), the RepaintManager, has invoked 
                  ( (Component) view_img ).repaint();
               -- JViewport.setViewPosition() may have invoked super.repaint()
            */
            super.repaint();

            Debug.println( "ViewportTime: view_img.getXaxisViewPosition() = "
                         + view_pt.x );
            Debug.println( "ViewportTime: [after] getViewPosition() = "
                         + super.getViewPosition() );
        }
        Debug.println( "comp_evt = " + evt );
        Debug.println( "ViewportTime: componentResized()'s END: " );
    }


    public void componentMoved( ComponentEvent evt ) 
    {
        Debug.println( "ViewportTime: componentMoved()'s START: " );
        Debug.println( "comp_evt = " + evt );
        Debug.println( "ViewportTime: componentMoved()'s END: " );
    }

    public void componentHidden( ComponentEvent evt ) 
    {
        Debug.println( "ViewportTime: componentHidden()'s START: " );
        Debug.println( "comp_evt = " + evt );
        Debug.println( "ViewportTime: componentHidden()'s END: " );
    }

    public void componentShown( ComponentEvent evt ) 
    {
        Debug.println( "ViewportTime: componentShown()'s START: " );
        Debug.println( "comp_evt = " + evt );
        Debug.println( "ViewportTime: componentShown()'s END: " );
    }

/*
    //  This is for Debugging Profiling
    public void paintComponent( Graphics g )
    {
        Debug.println( "ViewportTime: paintComponent()'s START: " );
        super.paintComponent( g );
        //  "( (Component) view_img ).repaint()" may have been invoked
        //  in JComponent's paint() method's paintChildren() ?!
        Debug.println( "ViewportTime: paintComponent()'s END: " );
    }
*/


    /*
        Implementation of HierarchyBoundsListener
    */
/*
    public void ancestorMoved( HierarchyEvent evt )
    {
        Debug.println( "ViewportTime: ancestorMoved()'s START: " );
        Debug.println( "hrk_evt = " + evt );
        Debug.println( "ViewportTime: ancestorMoved()'s END: " );
    }

    public void ancestorResized( HierarchyEvent evt )
    {
        Debug.println( "ViewportTime: ancestorResized()'s START: " );
        Debug.println( "hrk_evt = " + evt );
        if ( view_img != null ) {
            Debug.println( "ViewportTime: view_img.getXaxisViewPosition() = "
                         + view_pt.x );
            Debug.println( "ViewportTime: [before] getViewPosition() = "
                         + super.getViewPosition() );
            Debug.println( "ViewportTime: ancestorMoved()'s this = "
                         + this );
            //  ScrollableImage.setJComponentSize(),
            //  JViewport.setPreferredSize() and JViewport.setViewPosition()
            //  need to be called when the topmost container in the
            //  containment hierarchy is
            //  resized but this class is moved but NOT resized.  In 
            //  this scenario, the resizing of topmost container seems 
            //  to reset the location of scrollable to (0,0) as well as 
            //  the size of the ScrollableImage to the visible size of 
            //  the JViewport.

            //  view_img.setJComponentSize();
            this.setPreferredSize( getSize() );
            view_pt.x = view_img.getXaxisViewPosition();
            super.setViewPosition( view_pt );

            // calling view.repaint() to ensure the view is repainted
            // after setViewPosition is called.
            // -- apparently, this.repaint(), the RepaintManager, has invoked 
            //    ( (Component) view_img ).repaint();
            // -- JViewport.setViewPosition() may have invoked super.repaint()

            super.repaint();
            Debug.println( "ViewportTime: view_img.getXaxisViewPosition() = "
                         + view_pt.x );
            Debug.println( "ViewportTime: [after] getViewPosition() = "
                         + super.getViewPosition() );
            Debug.println( "ViewportTime: ancestorMoved()'s this = "
                         + this );
        }
        Debug.println( "ViewportTime: ancestorResized()'s END: " );
    }
*/
}
