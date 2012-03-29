function F=frameaccel(F,Ls);  
%FRAMEACCEL  Precompute structures
%   Usage: F=frameaccel(F,L);
%
%   `F=frameaccel(F,Ls)` precomputes certain structures that makes the basic
%   frame operations |frana|_ and |frsyn|_ faster (like instantiating the
%   window from a textual description). If you only need to call the
%   routines once, calling `frameaccel` first will not provide any total
%   gain, but if you are repeatedly calling these routines, for instance in
%   an iterative algorithm, is will be a benefit.
%
%   Notice that you need to input the signal length *Ls*, so this routines
%   is only a benefit if *Ls* stays fixed.
%
%   See also: newframe, frana, framelengthsignal, framelengthcoef
  
if ~isfield(F,'ga')
  % Quick exit, the transform does not use analysis or synthesis
  % windows.
  return;
end;
  
% From this point and on, we are sure that F.ga and F.gs exists.

L=framelengthsignal(F,Ls);

if ~isempty(F.ga)
  
  switch(F.type)
   case {'dgt','dgtreal'}
    [F.ga,F.ga_info]  = gabwin(F.ga,F.a,F.M,L);
   case {'dwilt','wmdct'}
    [F.ga,F.ga_info]  = wilwin(F.ga,F.M,L);
   case {'filterbank','ufilterbank'}
    [F.ga,F.ga_info]  = filterbankwin(F.ga,F.a,L);
   case {'filterbankreal','ufilterbankreal'}
    [F.ga,F.ga_info]  = filterbankwin(F.ga,F.a,L,'real');
  end;
  
end;

if ~isempty(F.gs)
  
  switch(F.type)
   case {'dgt','dgtreal'}
    [F.gs,F.gs_info] = gabwin(F.gs,F.a,F.M,L);
   case {'dwilt','wmdct'}
    [F.gs,F.gs_info] = wilwin(F.gs,F.M,L);
   case {'filterbank','ufilterbank'}
    [F.gs,F.gs_info]  = filterbankwin(F.gs,F.a,L);
   case {'filterbankreal','ufilterbankreal'}
    [F.gs,F.gs_info]  = filterbankwin(F.gs,F.a,L,'real');

  end;
  
end;

F.L=L;

  