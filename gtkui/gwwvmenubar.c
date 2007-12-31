#include <gtk/gtk.h>
#include "gwwvmenubar.h"

#define BORDER_SPACING	0		/* Must match value in gtkmenubar.c */

/* This implements a "variable-sized" menubar */
/* If the window shrinks below the size needed to display all the components   */
/*  then it will display as many components as it can, and then add a final    */
/*  arrow which will be attached to a pulldown menu containing any undisplayed */
/*  components. */

static GtkWidgetClass       *parent_class = NULL;

static GtkShadowType get_shadow_type (GtkMenuBar *menubar) {
  GtkShadowType shadow_type = GTK_SHADOW_OUT;
  
  gtk_widget_style_get (GTK_WIDGET (menubar),
			"shadow_type", &shadow_type,
			NULL);

  return shadow_type;
}

static void gww_vmenu_bar_size_request (GtkWidget      *widget,
				       GtkRequisition *requisition) {
    GwwVMenuBar  *menu_bar;
    GtkMenuShell *menu_shell;
    GtkRequisition child_requisition;
    gint ipadding;
    GtkWidget *child;
    GList *children;

    g_return_if_fail (GWW_IS_VMENU_BAR (widget));
    g_return_if_fail (requisition != NULL);

    requisition->width = 0;
    requisition->height = 0;
  
    if (GTK_WIDGET_VISIBLE (widget)) {
	menu_bar = GWW_VMENU_BAR (widget);
	menu_shell = GTK_MENU_SHELL (widget);

	gtk_widget_size_request (menu_bar->extra_arrow, requisition);

	for ( children = menu_bar->children; children!=NULL; children=children->next ) {
	    child = children->data;

	    if (GTK_WIDGET_VISIBLE (child)) {
		gtk_widget_size_request (child, &child_requisition);
		requisition->height = MAX (requisition->height, child_requisition.height);
	    }
	}

	gtk_widget_style_get (widget, "internal_padding", &ipadding, NULL);
      
	requisition->width += (GTK_CONTAINER (menu_bar)->border_width +
				ipadding + 
				BORDER_SPACING) * 2;
	requisition->height += (GTK_CONTAINER (menu_bar)->border_width +
				ipadding +
				BORDER_SPACING) * 2;

	if (get_shadow_type ( GTK_MENU_BAR(menu_bar)) != GTK_SHADOW_NONE) {
	    requisition->width += widget->style->xthickness * 2;
	    requisition->height += widget->style->ythickness * 2;
	}
    }
}

static void gww_vmenu_bar_size_allocate (GtkWidget     *widget,
					GtkAllocation *allocation) {
    GwwVMenuBar *vmenu_bar;
    GtkRequisition arrow_requisition, child_requisition;
    GList *children;
    gint i, goodi, extra_active;
    gint width, total_width;
    gint ipadding;

    g_return_if_fail (GWW_IS_VMENU_BAR (widget));
    g_return_if_fail (allocation != NULL);

    vmenu_bar = GWW_VMENU_BAR (widget);

    /* First figure out what widgets we want to display under the new */
    /*  allocation scheme */
    gtk_widget_size_request (vmenu_bar->extra_arrow, &arrow_requisition);

    total_width = allocation->width;
    gtk_widget_style_get (widget, "internal_padding", &ipadding, NULL);
    total_width -= (GTK_CONTAINER (vmenu_bar)->border_width +
				ipadding + 
				BORDER_SPACING) * 2; 
    if (get_shadow_type ( GTK_MENU_BAR(vmenu_bar)) != GTK_SHADOW_NONE)
	total_width -= widget->style->xthickness * 2;

    goodi = -1;
    width = 0;
    for ( i=0, children = vmenu_bar->children; children!=NULL ;
	    ++i, children=children->next ) {
	if (!GTK_WIDGET_VISIBLE (children->data))
    continue;
	if ( width+arrow_requisition.width <= total_width ||
		( children->next==NULL && width <= total_width ))
	    goodi = i-1;
	gtk_widget_size_request (children->data, &child_requisition);
	if ( width + child_requisition.width > total_width )
    break;
	width += child_requisition.width + 6;	/* Why do I need a fudge factor of 6? I'm doing something wrong */
    }
    if ( children==NULL ) ++goodi;
    extra_active = ( children!=NULL );
    if ( vmenu_bar->child_last==goodi &&
	    vmenu_bar->extra_active == extra_active ) {
	if ( widget->allocation.x == allocation->x &&
		widget->allocation.y == allocation->y &&
		widget->allocation.width == allocation->width &&
		widget->allocation.height == allocation->height )
return;
    } else {
	/* Then undo any previous allocation */
	for ( i=0, children = vmenu_bar->children; children!=NULL && i<=vmenu_bar->child_last;
		++i, children=children->next ) {
	    ((GtkContainerClass*) parent_class)->remove( GTK_CONTAINER(vmenu_bar),
			children->data );
	}

	if ( vmenu_bar->extra_active ) {
	    ((GtkContainerClass*) parent_class)->remove( GTK_CONTAINER(vmenu_bar),
			vmenu_bar->extra_arrow );
	    for ( ; children!=NULL ; children=children->next ) {
		gtk_container_remove( GTK_CONTAINER(vmenu_bar->extra_menu),
			    children->data );
	    }
	}

	/* Then configure the new scheme */
	vmenu_bar->extra_active = extra_active;
	vmenu_bar->child_last = goodi;

	for ( i=0, children = vmenu_bar->children; children!=NULL && i<=goodi;
		++i, children=children->next ) {
	    ((GtkContainerClass*) parent_class)->add( GTK_CONTAINER(vmenu_bar),
			children->data );
	}

	if ( vmenu_bar->extra_active ) {
	    ((GtkContainerClass*) parent_class)->add( GTK_CONTAINER(vmenu_bar),
			vmenu_bar->extra_arrow );
	    for ( ; children!=NULL ; children=children->next ) {
		gtk_container_add( GTK_CONTAINER(vmenu_bar->extra_menu),
			    children->data );
	    }
	}
    }

    /* Finally ask the underlying menubar to do whatever it needs to do */
    /*  with the widget set which we chose to be active */
    ((GtkWidgetClass*) parent_class)->size_allocate( GTK_WIDGET(vmenu_bar),
		allocation );
}

static void gww_vmenu_bar_add(  GtkContainer *widget,
				GtkWidget    *child) {
    GwwVMenuBar *vmenu_bar;

    g_return_if_fail (GWW_IS_VMENU_BAR (widget));
    vmenu_bar = GWW_VMENU_BAR (widget);

    vmenu_bar->children = g_list_insert (vmenu_bar->children, child, -1);	/* Add to end */
    g_object_ref( G_OBJECT(child));

    /*gtk_widget_set_parent (child, GTK_WIDGET (vmenu_bar));*/
    /* Don't set the parent until we actually display it */

    /* BUG!!! I am presuming that any adds/removes will be done before */
    /*  the widget is actually displayed */
}

static void gww_vmenu_bar_remove( GtkContainer *widget,
				  GtkWidget    *child) {
    GwwVMenuBar *vmenu_bar;

    g_return_if_fail (GWW_IS_VMENU_BAR (widget));
    if ( child->parent!=NULL )
	((GtkContainerClass*) parent_class)->remove( GTK_CONTAINER(child->parent),
		    child );
    vmenu_bar = GWW_VMENU_BAR (widget);

    vmenu_bar->children = g_list_remove (vmenu_bar->children, child);
    /* BUG!!! I am presuming that any adds/removes will be done before */
    /*  the widget is actually displayed */
    g_object_unref( G_OBJECT(child));
}


static void gww_vmenu_bar_class_init (GwwVMenuBarClass *class) {
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    parent_class = g_type_class_peek_parent (class);
  
    widget_class    = (GtkWidgetClass*) class;
    container_class = (GtkContainerClass*) class;

    widget_class->size_request = gww_vmenu_bar_size_request;
    widget_class->size_allocate = gww_vmenu_bar_size_allocate;

    container_class->add    = gww_vmenu_bar_add;
    container_class->remove = gww_vmenu_bar_remove;
}

static void gww_vmenu_bar_init (GwwVMenuBar *mbar) {
    GtkWidget *arrow;

    arrow = gtk_arrow_new(GTK_ARROW_DOWN,GTK_SHADOW_OUT);
    gtk_widget_show( arrow );
    mbar->extra_arrow = gtk_menu_item_new();
    gtk_widget_show( mbar->extra_arrow );
    gtk_container_add ( GTK_CONTAINER(mbar->extra_arrow), arrow);
    mbar->extra_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mbar->extra_arrow), mbar->extra_menu);
    g_object_ref( G_OBJECT( mbar->extra_arrow ));

    mbar->children     = NULL;
    mbar->child_last   = -1;
    mbar->extra_active = FALSE;
}

GType gww_vmenu_bar_get_type (void) {
    static GType vmenu_bar_type = 0;

  if (!vmenu_bar_type)
    {
      static const GTypeInfo vmenu_bar_info =
      {
	sizeof (GtkMenuBarClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gww_vmenu_bar_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GwwVMenuBar),
	0,		/* n_preallocs */
	(GtkObjectInitFunc) gww_vmenu_bar_init,
      };

      vmenu_bar_type = g_type_register_static (GTK_TYPE_MENU_BAR, "GwwVMenuBar",
					      &vmenu_bar_info, 0);
    }

  return vmenu_bar_type;
}

GtkWidget *gww_vmenu_bar_new (void)
{
  return g_object_new (GWW_TYPE_VMENU_BAR, NULL);
}
