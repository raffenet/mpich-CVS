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
import java.net.*;
import javax.swing.*;

import base.drawable.TimeBoundingBox;
import viewer.common.Const;
import viewer.common.Dialogs;

public class ActionZoomRedo implements ActionListener
{
    private TimelineToolBar    toolbar;
    private ModelTime          model;

    public ActionZoomRedo( TimelineToolBar in_toolbar, ModelTime in_model )
    {
        toolbar    = in_toolbar;
        model      = in_model;
    }

    public void actionPerformed( ActionEvent event )
    {
        if ( model.isZoomRedoStackEmpty() ) {
            Frame frame = (Frame) SwingUtilities.windowForComponent( toolbar );
            String msg = "Zoom Redo Stack is empty";
            Dialogs.warn( frame, msg );
        }
        else
            model.zoomRedo();

        // Set toolbar buttons to reflect status
        if ( toolbar != null )
            toolbar.resetZoomButtons();

        if ( Debug.isActive() )
            Debug.println( "Action for Zoom Redo button." );
    }
}
