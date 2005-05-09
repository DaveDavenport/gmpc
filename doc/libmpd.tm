<TeXmacs|1.0.4>

<style|<tuple|help|title-generic>>

<\body>
  <\make-title>
    <title|LibMpd>

    <author|Qball Cow>

    <title-email|Qball@qballcow.nl>
  </make-title>

  <section|History: >

  libmpd is very new, I wrote it to abstract the mpd abstraction out of the
  code in gmpc. Therefor making gmpc code easier to read, but even more
  important easier to debug. Libmpd will now be released as a set of C files.
  The goal is to make it a seperate library. Comments/suggestions are more
  then welcome, just keep in mind this is a pre-release.\ 

  e-mail me at: qball@qballcow.nl

  <section|Goal: ><with|color|black|<with|color|black|>>

  Trying to provide a easy to use high level, callback based access to mpd.
  It tries to be fast and keep the data transfer with mpd to the minimum.
  Todo this it implement qeues for deleting and adding songs. There is extra
  functionallity added for the eas of the programmer.\ 

  <with|color|red|more bla bla here>

  It's written in plain C and uses the following header files

  <\itemize-dot>
    <item>regex.h

    <item>libmpdclient.h\ 

    <item>stdio.h\ 

    <item>stdlib.h (using __USE_GNU)\ 

    <item>debug_printf.h (should come along with libmpd)
  </itemize-dot>

  <section|Status>

  <subsection| Implemented:>

  <\itemize-dot>
    <item>Basic player access.

    <item>Playlist access.

    <item>Error handling.

    <item>Command queues.

    <item>Advanced search.
  </itemize-dot>

  <subsection|ToDo:>\ 

  <\itemize-dot>
    <item>Permission check.

    <item>translation.

    <item>auth after connect

    <item>Abstract more from libmpdclient. I still return structures and
    defines from libmpdclient.

    <item>Become an Independent <with|language|british|>library.\ 

    <item>More functionallity

    <item>etc.
  </itemize-dot>

  \;

  <section|Error behaviour: >

  libmpd checks after every command for possible error's. If an error occured
  libmpd wil disconnect and call the error callback. The error callback will
  contain the error Id, and a error string.

  <section|Api:>

  The main object is the MpdOb struct. The user should in <strong|NO> way use
  any of the internal values. These can be invalid at any given time. The
  MpdOb can be created using the following functions:

  <subsection|Basic:>

  All these objects are after creation not yet connected to mpd

  <\code>
    MpdObj * mpd_ob_new_default ();\ 
  </code>

  This creates a new object with hostname="localhost" and port="6600" This
  function always returns an object. (given malloc doesnt fail)

  <\code>
    MpdObj * mpd_ob_new (char *hostname, int port, char *password);\ 
  </code>

  This creates a object with given hostname,port and password. If password is
  NULL, no password is sended to mpd.

  <\code>
    void mpd_ob_set_hostname (MpdObj *mi, char *hostname);\ 
  </code>

  Set the hostname. (default localhost)

  <\code>
    void mpd_ob_set_password (MpdObj *mi, char *password);\ 
  </code>

  Set the password. To unset the password pass NULL as argument.

  <\code>
    void mpd_ob_set_port (MpdObj *mi, int port);\ 
  </code>

  Set the port number. (default 6600)

  <\code>
    void mpd_ob_set_connection_timeout (MpdObj *mi, float timeout);
  </code>

  Set the connection timeout.

  <\code>
    int mpd_ob_connect (MpdObj *mi);\ 
  </code>

  Connect to mpd.

  <\code>
    int mpd_ob_disconnect (MpdObj *mi);
  </code>

  Disconnect from mpd. This will free all buffered data, including command
  queues.

  <\code>
    int mpd_ob_check_connected (MpdObj *mi);\ 
  </code>

  returns TRUE when connected

  <code*|int mpd_ob_check_error (MpdObj *mi); >

  returns TRUE on error.
</body>

<\initial>
  <\collection>
    <associate|color|black>
  </collection>
</initial>

<\references>
  <\collection>
    <associate|auto-1|<tuple|1|1>>
    <associate|auto-10|<tuple|8|?>>
    <associate|auto-11|<tuple|8.1|?>>
    <associate|auto-2|<tuple|2|1>>
    <associate|auto-3|<tuple|3|1>>
    <associate|auto-4|<tuple|3.1|1>>
    <associate|auto-5|<tuple|3.2|1>>
    <associate|auto-6|<tuple|4|2>>
    <associate|auto-7|<tuple|5|2>>
    <associate|auto-8|<tuple|5.1|2>>
    <associate|auto-9|<tuple|7|?>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|History:
      > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-1><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Goal:
      > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-2><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Status>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-3><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>| Implemented:
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-4>>

      <with|par-left|<quote|1.5fn>|ToDo: <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-5>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Error
      behaviour: > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-6><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Api:>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-7><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|Basic:
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <pageref|auto-8>>
    </associate>
  </collection>
</auxiliary>