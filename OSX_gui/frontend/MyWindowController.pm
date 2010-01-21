package MyWindowController;

use Foundation;
use Foundation::Functions;
use AppKit;
use AppKit::Functions;
use CustomClass;

@ISA = qw(Exporter);

$context = undef;

sub new {
    # Typical Perl constructor
    # See 'perltoot' for details
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = {};

    # Outlets
    $self->{'Window'} = undef;
    $self->{'Configuration'} = undef;
    $self->{'OpenGLView'} = undef;

    bless ($self, $class);
    
    $self->{'NSWindowController'} = NSWindowController->alloc->initWithWindowNibName_owner("MainWindow", $self);
    $self->{'NSWindowController'}->window;
    $self->{'pixelFormat'} = CustomClass->alloc->init->getFormat();
    $self->{'context'} = NSOpenGLContext->alloc->initWithFormat_shareContext($self->{'pixelFormat'}, undef);
    $self->{'context'}->setView($self->{'OpenGLView'});
    $self->{'context'}->makeCurrentContext;
    $context = $self->{'context'};
    $view = $self->{'context'}->view;
    $self->{'Configuration'}->setFreeWRLView($self->{'OpenGLView'});
    $self->{'OpenGLView'}->setController($self->{'Configuration'});
    #print "set controller to : $self->{'Configuration'}\n";
    
    $currentcontext = NSOpenGLContext->currentContext;
    return $self;
}

1;

