/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.util.Stack;
import javax.swing.JTree;

public class NestingStacks
{
    //  0.0 < Nesting_Height_Reduction < 1.0
    private static  float  Nesting_Height_Reduction = 0.8f;
    private static  float  Initial_Nesting_Height   = 0.8f;
    private static  float  Shadow_Nesting_Height    = 0.4f;

    private JTree          tree_view;
    private Stack[]        nesting_stacks;
    private boolean        hasNestingStacksBeenUsed;
    private boolean        isTimeBoxScrolling;

    private int            num_rows;

    public NestingStacks( final JTree  in_tree_view )
    {
        tree_view       = in_tree_view;
        nesting_stacks  = null;
    }

    public static void  setNestingHeightReduction( float new_reduction )
    {
        if ( new_reduction > 0.0f && new_reduction < 1.0f )
            Nesting_Height_Reduction = new_reduction;
    }

    public static void  setInitialNestingHeight( float new_reduction )
    {
        if ( new_reduction > 0.0f && new_reduction < 1.0f ) {
            Initial_Nesting_Height   = Nesting_Height_Reduction;   
            Shadow_Nesting_Height    = Initial_Nesting_Height / 2.0f;
        }
    }

    public static float getShadowNestingHeight()
    {
        return Shadow_Nesting_Height;
    }

    public void initialize( boolean isScrolling )
    {
        //  Need to check to see if tree_view has been updated,
        //  If not, no need to construct all these Stack[].
        isTimeBoxScrolling = isScrolling;
        num_rows           = tree_view.getRowCount();
        nesting_stacks     = new Stack[ num_rows ];
        for ( int irow = 0 ; irow < num_rows ; irow++ ) {
            //  Select only non-expanded row
            if ( ! tree_view.isExpanded( irow ) )
                nesting_stacks[ irow ] = new Stack();
            else
                nesting_stacks[ irow ] = null;
        }
        hasNestingStacksBeenUsed = false;
    }

    public void reset()
    {
        if ( hasNestingStacksBeenUsed ) {
            for ( int irow = 0 ; irow < num_rows ; irow++ ) {
                //  Select only non-expanded row
                if ( nesting_stacks[ irow ] != null )
                    nesting_stacks[ irow ].clear();
            }
        }
        else
            hasNestingStacksBeenUsed = true;
    }

    public void finalize()
    {
        for ( int irow = 0 ; irow < num_rows ; irow++ ) {
            //  Select only non-expanded row
            if ( nesting_stacks[ irow ] != null )
                nesting_stacks[ irow ] = null;
        }
    }

    public boolean isReadyToGetNestingFactorFor( Primitive cur_prime )
    {
        if ( this.isTimeBoxScrolling )
            return cur_prime.isNestingFactorUninitialized();
        else
            return true;
    }

    /*
       Given the NestingStack of the timeline that the Drawable is on,
       Compute the NestingFactor, nesting_ftr.

       It seems NestingFactor cannot be cached, as Zoom-In operations
       bring in the real Drawables that are represented by the Shadows.
       These Drawables needs all the other real Drawables to be presented
       on the NestingStack to have their NestingFactor computed correctly.

       If the input Primitive is Shadow, nesting_stack ignores its presence.
       i.e. Shadow won't be pushed onto the stack.
     */
    public float getNestingFactorFor( final Primitive cur_prime )
    {
        boolean   isRealDrawable  = ! ( cur_prime instanceof Shadow );
        int       rowID           = cur_prime.getRowID();
        Stack     nesting_stack   = nesting_stacks[ rowID ];
        float     nesting_ftr     = Drawable.NON_NESTABLE;
        Primitive prime;
        /*
           Assume all drawables are arranged in increasing starttime order
           nesting_stack still needs to pop() even if the input Primitive,
           shadow, is excluded from being push() to the stack, because 
           when shadow overlaps with one of real drawables on the stack.
           Drawables before the shadow are completely disjointed from
           the shadow, so they should be pop() off the stack.
        */
        while ( ! nesting_stack.empty() ) {
            prime = (Primitive) nesting_stack.peek();
            // cur_prime is nested inside or is overlap with prime
            if ( cur_prime.getEarliestTime() < prime.getLatestTime() ) {
                nesting_ftr = prime.getNestingFactor()
                            * Nesting_Height_Reduction;
                if ( isRealDrawable )
                    nesting_stack.push( cur_prime );
                return nesting_ftr;
            }
            else
                nesting_stack.pop();
        }
        // i.e.  nesting_stack.empty() == true
        nesting_ftr = Initial_Nesting_Height;
        if ( isRealDrawable )
            nesting_stack.push( cur_prime );
        return nesting_ftr;
    } 
}
