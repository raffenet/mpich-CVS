/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */
package viewer.legends;

import java.util.Comparator;

import base.drawable.Topology;
import base.drawable.Category;

public class LegendComparators
{
    public static final Comparator TOPO_ORDER
                                   = new TopologyOrder();
    public static final Comparator INDEX_ORDER
                                   = new IndexOrder();
    public static final Comparator PREVIEW_ORDER
                                   = new PreviewOrder();
    public static final Comparator CASE_INSENSITIVE_ORDER
                                   = new CaseInsensitiveOrder();
    public static final Comparator CASE_SENSITIVE_ORDER
                                   = new CaseSensitiveOrder();
    public static final Comparator CASE_INSENSITIVE_TOPO_ORDER
                                   = new CaseInsensitiveTopologyOrder();
    public static final Comparator CASE_SENSITIVE_TOPO_ORDER
                                   = new CaseSensitiveTopologyOrder();

    public static class TopologyOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Topology topo1 = (Topology) o1;
            Topology topo2 = (Topology) o2;
            return topo2.hashCode() - topo1.hashCode();
            // intentionally reversed, so arrow < state < event
        }
    }

    public static class IndexOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1 = (Category) o1;
            Category type2 = (Category) o2;
            return type1.getIndex() - type2.getIndex();
        }
    }

    /*
       This comparator gives preference over Preview drawable
       All Preview object's category indexes are negative as defined in
       logformat/clogTOdrawable/InputLog.java & logformat/trace/InputLog.java.
    */
    public static class PreviewOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1  = (Category) o1;
            Category type2  = (Category) o2;
            int      pview1 = ( type1.getIndex() < 0 ? 0 : 1 );
            int      pview2 = ( type2.getIndex() < 0 ? 0 : 1 );
            return pview1 - pview2;
        }
    }

    public static class CaseSensitiveOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1      = (Category) o1;
            Category type2      = (Category) o2;
            int      pview_diff = PREVIEW_ORDER.compare( type1, type2 );
            if ( pview_diff != 0 )
                return pview_diff;
            else
                return type1.getName().compareTo( type2.getName() );
        }
    }

    public static class CaseInsensitiveOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1      = (Category) o1;
            Category type2      = (Category) o2;
            int      pview_diff = PREVIEW_ORDER.compare( type1, type2 );
            if ( pview_diff != 0 )
                return pview_diff;
            else
                return type1.getName().compareToIgnoreCase( type2.getName() );
        }
    }

    public static class CaseSensitiveTopologyOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1     = (Category) o1;
            Category type2     = (Category) o2;
            int      topo_diff = TOPO_ORDER.compare( type1.getTopology(),
                                                     type2.getTopology() );
            if ( topo_diff != 0 )
                return topo_diff;
            else
                return CASE_SENSITIVE_ORDER.compare( type1, type2 );
        }
    }

    public static class CaseInsensitiveTopologyOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            Category type1     = (Category) o1;
            Category type2     = (Category) o2;
            int      topo_diff = TOPO_ORDER.compare( type1.getTopology(),
                                                     type2.getTopology() );
            if ( topo_diff != 0 )
                return topo_diff;
            else
                return CASE_INSENSITIVE_ORDER.compare( type1, type2 );
        }
    }
}
