/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.Component;

/*
   Define the interface to be implemented by the view object, ScrollableView,
   so that it is searchable in time
*/

public interface SearchableView
{
    public Component searchNextComponent( boolean isNewSearch );

    public Component searchPreviousComponent( boolean isNewSearch );
}
