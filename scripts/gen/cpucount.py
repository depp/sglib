
def cpu_count():
    ncpu = None
    try:
        import multiprocessing
    except ImportError:
        pass
    else:
        ncpu = multiprocessing.cpu_count()
    if ncpu is None:
        if hasattr(os, "sysconf"):
            if 'SC_NPROCESSORS_ONLN' in os.sysconf_names:
                ncpu = os.sysconf('SC_NPROCESSORS_ONLN')
            elif 'NUMBER_OF_PROCESSORS' in os.environ:
                ncpu = int(os.environ['NUMBER_OF_PROCESSORS'])
    if ncpu is not None and ncpu >= 1:
        return ncpu
    return 1
