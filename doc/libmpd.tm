<TeXmacs|1.0.5>

<style|help>

<\body>
  <doc-data|<doc-title|LibMpd>|<doc-date|<date>>|<doc-author-data|<author-name|Qball>|<author-email|Qball@qballcow.nl>>>

  <section|History: >

  libmpd is very new, I wrote it to abstract the mpd abstraction out of the
  code in gmpc. Therefor making gmpc code easier to read, but even more
  important easier to debug. Libmpd will now be released as a set of C files.
  The goal is to make it a seperate library. Comments/suggestions are more
  then welcome, just keep in mind this is a pre-release. e-mail me at:
  qball@qballcow.nl

  <section|Goal: >

  Trying to provide a easy to use high level, callback based access to mpd.
  It tries to be fast and keep the data transfer with mpd to the minimum.
  Todo this it implement qeues for deleting and adding songs. There is extra
  functionallity added for the eas of the programmer.\ 

  <more>bla bla here

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

  The main object is the MpdOb struct. The user should in NO way use any of
  the internal values. These can be invalid at any given time. The MpdOb can
  be created using the following functions:

  <subsection|Basic:>

  All these objects are after creation not yet connected to mpd
</body>

<\references>
  <\collection>
    <associate|auto-1|<tuple|1|?>>
    <associate|auto-10|<tuple|8|?>>
    <associate|auto-11|<tuple|8.1|?>>
    <associate|auto-2|<tuple|2|?>>
    <associate|auto-3|<tuple|3|?>>
    <associate|auto-4|<tuple|3.1|?>>
    <associate|auto-5|<tuple|3.2|?>>
    <associate|auto-6|<tuple|4|?>>
    <associate|auto-7|<tuple|5|?>>
    <associate|auto-8|<tuple|5.1|?>>
    <associate|auto-9|<tuple|7|?>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|History:
      > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-1><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Goal:
      > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Status>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-3><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>| Implemented:
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-4>>

      <with|par-left|<quote|1.5fn>|ToDo: <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-5>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Error
      behaviour: > <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-6><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Api:>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-7><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|Basic:
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-8>>
    </associate>
  </collection>
</auxiliary>