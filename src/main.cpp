#include <iostream>
#include <libupnp-tools/UPnPControlPoint.hpp>
#include <gtk/gtk.h>

using namespace std;
using namespace osl;
using namespace upnp;

void
quick_message (GtkWindow *parent, const gchar *message)
{
    GtkWidget *dialog, *label, *content_area;
    GtkDialogFlags flags;

    // Create the widgets
    flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    dialog = gtk_dialog_new_with_buttons ("Message",
					  parent,
					  flags,
					  "_OK",
					  GTK_RESPONSE_NONE,
					  NULL);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    label = gtk_label_new (message);

    // Ensure that the dialog box is destroyed when the user responds

    g_signal_connect_swapped (dialog,
			      "response",
			      G_CALLBACK (gtk_widget_destroy),
			      dialog);

    // Add the label, and show everything we’ve added

    gtk_container_add (GTK_CONTAINER (content_area), label);
    gtk_widget_show_all (dialog);
}

GtkWidget * create_list(GtkListStore * model, GtkCallback on_click)
{
    GtkWidget * scrolled_window;
    GtkWidget * tree_view;
    GtkTreeIter iter;
    GtkCellRenderer * cell;
    GtkTreeViewColumn * column;
    GtkTreeSelection * select;
    int i;

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

    tree_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(model));
    gtk_widget_show(tree_view);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Devices", cell, "text", 0, NULL);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(on_click), NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view),
				GTK_TREE_VIEW_COLUMN(column));

    return scrolled_window;
}

GtkWidget * create_text()
{

    GtkWidget * scrolled_window;
    GtkWidget * view;
    GtkTextBuffer * buffer;

    view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(scrolled_window), view);
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    gtk_text_buffer_insert(buffer, &iter,
			   "From: pathfinder@nasa.gov\n"
			   "To: mom@nasa.gov\n"
			   "Subject: Made it!\n"
			   "\n"
			   "We just go in this morning. The weather has been\n"
			   "great - clear but cold, and there are lots of fun sights.\n"
			   "sojourner says hi. See you soon.\n"
			   " -Path\n", -1);

    gtk_widget_show_all(scrolled_window);
    return scrolled_window;
}

class Window
{
private:
    GtkWidget * window;
    GtkWidget * hpaned;
    GtkListStore * model;
    GtkTreeIter iter;
    AutoRef<UPnPDevice> device;
public:
    Window() {
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window), "key-release-event", G_CALLBACK(Window::on_key_press), this);

	hpaned = gtk_hpaned_new();
	GtkWidget * widget;
	gtk_container_add(GTK_CONTAINER(window), hpaned);
	gtk_widget_show(hpaned);

	model = gtk_list_store_new(1, G_TYPE_STRING);

	// pane1
	widget = create_list(model, Window::on_select_device);
	gtk_paned_add1(GTK_PANED(hpaned), widget);
	gtk_widget_set_size_request(widget, 200, -1);
	gtk_widget_show(widget);

	// pane2
	widget = create_text();
	gtk_paned_add2(GTK_PANED(hpaned), widget);
	gtk_widget_show(widget);
    }
    
    virtual ~Window() {
    }

    void addDevice(AutoRef<UPnPDevice> device) {
	this->device = device;
    }

    void removeDevice(AutoRef<UPnPDevice> device) {
    }

    void update() {
	if (device.nil() == false) {
	    gchar * msg = g_strdup_printf("%s", device->friendlyName().c_str());
	    
	    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	    gtk_list_store_set(GTK_LIST_STORE(model),
			       &iter,
			       0, msg,
			       -1);
	    g_free(msg);
	    device = NULL;
	}
	
    }
    
    void move(int x, int y) {
	gtk_window_move(GTK_WINDOW(window), x, y);
    }

    void resize(int width, int height) {
	gtk_window_resize(GTK_WINDOW(window), width, height);
    }
    
    void setTitle(const string & title) {
	gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    }
    
    void show() {
	gtk_widget_show(window);
    }
    
    void close() {
	gtk_widget_destroy(GTK_WIDGET(window));
    }

    void setOnDestroy(GtkCallback on_destroy, gpointer user) {
	gtk_signal_connect(GTK_OBJECT(window), "destroy", G_CALLBACK(on_destroy), user);
    }

    virtual gboolean onKeyPress(GdkEventKey * event) {
	cout << "key press -- " << gdk_keyval_name(event->keyval) << endl;
	switch (event->keyval) {
	case 'q':
	    close();
	    return TRUE;
	case 'x':
	    quick_message(GTK_WINDOW(window), "message");
	    return TRUE;
	default:
	    break;
	}
	
	return FALSE;
    }
    
    static gboolean on_key_press(GtkWidget * widget, GdkEventKey * event, gpointer data) {
	Window * window = (Window*)data;
	return window->onKeyPress(event);
    }

    static void on_select_device(GtkWidget * widget, gpointer user) {
	GtkTreeIter iter;
	GtkTreeModel * model;
	gchar * text;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
	    gtk_tree_model_get(model, &iter, 0, &text, -1);
	    g_print("selected -- '%s'\n", text);
	    g_free(text);
	}
    }
};


class DeviceListener : public UPnPDeviceListener
{
private:
    Window * window;
public:
    DeviceListener(Window * window) : window(window) {
    }
    virtual ~DeviceListener() {
    }
    virtual void onDeviceAdded(AutoRef<UPnPDevice> device) {
	cout << "added: " << device->friendlyName() << endl;
	window->addDevice(device);
    }
    virtual void onDeviceRemoved(AutoRef<UPnPDevice> device) {
	cout << "removed: " << device->friendlyName() << endl;
	window->removeDevice(device);
    }
};


class Context
{
private:
    UPnPControlPoint * cp;
    Window * window;
    bool _finishing;
public:
    Context(int argc, char * argv[]) : _finishing(false) {
	gtk_init(&argc, &argv);
    }
    
    virtual ~Context() {
	delete window;
    }

    void onStart() {
	window = new Window;
	window->move(10, 10);
	window->resize(640, 480);
	window->setOnDestroy(Context::onDestroy, this);

	cp = new UPnPControlPoint(UPnPControlPoint::Config(0));
	cp->setDeviceListener(AutoRef<UPnPDeviceListener>(new DeviceListener(window)));
	cp->startAsync();
	cp->sendMsearchAsync("ssdp:all", 3);
    }

    void onStop() {
	cp->stop();
    }

    void run() {
	onStart();
	window->show();
	while (_finishing == false) {
	    if (gtk_events_pending()) {
		gtk_main_iteration();
	    }
	    window->update();
	}
	onStop();
    }

    void finish() {
	_finishing = true;
    }

    static void onDestroy(GtkWidget * widget, gpointer user) {
	Context * context = (Context*)user;
	context->finish();
    }
};


int main(int argc, char *argv[])
{
    Context context(argc, argv);
    context.run();
    return 0;
}
