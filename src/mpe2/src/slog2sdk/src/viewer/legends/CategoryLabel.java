/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.legends;

import java.awt.Dimension;
import javax.swing.JLabel;
import javax.swing.SwingConstants;

import base.drawable.Category;

public class CategoryLabel extends JLabel
{
    public CategoryLabel( final Category objdef )
    {
        super( objdef.getName(), new CategoryIcon( objdef ),
               SwingConstants.LEFT );
        super.setIconTextGap( 2 * Const.CELL_ICON_TEXT_GAP );
    }

    public Dimension getPreferredSize()
    {
        Dimension pref_sz = super.getPreferredSize();
        return new Dimension( pref_sz.width, Const.CELL_HEIGHT );
    }
}
