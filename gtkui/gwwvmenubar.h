#ifndef __GWW_VMENU_BAR_H__
#define __GWW_VMENU_BAR_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	GWW_TYPE_VMENU_BAR               (gww_vmenu_bar_get_type ())
#define GWW_VMENU_BAR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWW_TYPE_VMENU_BAR, GwwVMenuBar))
#define GWW_VMENU_BAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), GWW_TYPE_VMENU_BAR, GwwVMenuBarClass))
#define GWW_IS_VMENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWW_TYPE_VMENU_BAR))
#define GWW_IS_VMENU_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), GWW_TYPE_VMENU_BAR))
#define GWW_VMENU_BAR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GWW_TYPE_VMENU_BAR, GwwVMenuBarClass))


typedef struct _GwwVMenuBar       GwwVMenuBar;
typedef struct _GwwVMenuBarClass  GwwVMenuBarClass;

struct _GwwVMenuBar
{
  GtkMenuBar menu_bar;

  GtkWidget *extra_arrow;
  GtkWidget *extra_menu;

  GList *children;
  gint   child_last;		/* Last child currently visible */
  guint  extra_active: 1;
};

struct _GwwVMenuBarClass
{
  GtkMenuBarClass parent_class;
};


GType      gww_vmenu_bar_get_type        (void) G_GNUC_CONST;
GtkWidget* gww_vmenu_bar_new             (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GWW_VMENU_BAR_H__ */
