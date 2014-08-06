function [c,info]=dtwfb(f,dualwt,varargin)
%DTWFB Dual-Tree Wavelet FilterBank
%   Usage:  c=dtwfb(f,dualwt);
%           c=dtwfb(f,{dw,J});
%           [c,info]=dtwfb(...);
%
%   Input parameters:
%         f      : Input data.
%         dualwt : Dual-tree Wavelet Filterbank definition.
%
%   Output parameters:
%         c    : Coefficients stored in a cell-array.
%         info : Additional transform parameters struct.
%
%   `c=dtwfbt(f,dualwt)` computes dual-tree complex wavelet coefficients 
%   of the signal *f*. The representation is approximately 
%   time-invariant and provides analytic behavior. Due to these facts, 
%   the resulting subbands are nearly aliasing free making them suitable 
%   for severe coefficient modifications. The representation is two times
%   redundant, provided critical subsampling of all involved filterbanks.
%
%   The shape of the filterbank tree and filters used is controlled by 
%   `dualwt` (for possible formats see below). The output *c* is a 
%   cell-array with each element containing a single subband. The subbands 
%   are ordered with increasing center frequency of the subband. 
%
%   In addition, the function returns struct. `info` containing transform 
%   parameters. It can be conviniently used for the inverse transform
%   |idtwfbreal| e.g. `fhat = idtwfbreal(c,info)`. It is also required by
%   the |plotwavelets| function.
%
%   If *f* is a matrix, the transform is applied to each column.
%
%   Two formats of `dualwt` are accepted:
% 
%   1) Cell array of parameters. First two elements of the array are 
%      mandatory `{dt,J}`. 
% 
%         `dt`   
%            Basic dual-tree filters
%         *J*
%            Number of levels of the filterbank tree
%
%      Possible formats of `dt` are the same as in |fwtinit| except the
%      `wfiltdt_` prefix is used when searching for function specifying
%      the actual impulse responses. These filters were designed specially
%      for the dual-tree filterbank to achieve the half-sample shift 
%      ultimatelly resulting in analytic (complex) behavior of the 
%      transform.
%      
%      The default shape of the filterbank tree is DWT i.e. only low-pass
%      output is decomposed further (*J* times in total).
%
%      Different filterbank tree shapes can be obtained by passing
%      additional flag in the cell array. Supported flags (mutually
%      exclusive) are:
%
%      `'dwt'`
%         Plain DWT tree (default). This gives one band per octave freq. 
%         resolution when using 2 channel basic wavelet filterbank.
%
%      `'full'`
%         Full filterbank tree. Both (all) basic filterbank outputs are
%         decomposed further up to depth *J* achieving linear frequency band
%         division.
%
%      `'doubleband'`,`'quadband'`,`'octaband'`
%         The filterbank is designed such that it mimics 4-band, 8-band or
%         16-band complex wavelet transform provided the basic filterbank
%         is 2 channel. In this case, *J* is treated such that it defines
%         number of levels of 4-band, 8-band or 16-band transform.
%
%      The dual-tree wavelet filterbank can use any basic wavelet
%      filterbank in the first stage of both trees, provided they are 
%      shifted by 1 sample (done internally). A custom first stage 
%      filterbank can be defined by passing the following
%      key-value pair in the cell array:
%
%      `'first'`,`w` 
%         `w` defines a regular basic filterbank. Accepted formats are the
%         same as in |fwtinit| assuming the `wfilt_` prefix.
%
%      Similarly, when working with a filterbank tree containing
%      decomposition of high-pass outputs, some filters in both trees must
%      be replaced by a regular basic filterbank in order to achieve the
%      aproximatelly analytic behavior. A custom filterbank can be
%      specified by passing another key-value pair in the cell array:
%
%      `'leaf'`,`w` 
%         `w` defines a regular basic filterbank. Accepted formats are the
%         same as in |fwtinit| assuming the `wfilt_` prefix.      
%
%   2) Another possibility is to pass directly a struct. returned by 
%      |dtwfbinit| and possibly modified by |wfbtremove|. 
%
%   Optional args.:
%   ---------------
%
%   In addition, the following flag groups are supported:
%
%   `'freq'`,`'nat'`
%      Frequency or natural (Paley) ordering of coefficient subbands.
%      By default, subbands are ordered according to frequency. The natural
%      ordering is how the subbands are obtained from the filterbank tree
%      without modifications. The ordering differ only in non-plain DWT
%      case.
%
%   Boundary handling:
%   ------------------
%
%   In contrast with |fwt|, |wfbt| and |wpfbt|, this function supports
%   periodic boundary handling only.
%
%   Examples:
%   ---------
%   
%   A simple example of calling the |dtwfb| function using the regular
%   DWT iterated filterbank. The second figure shows a magnitude frequency
%   response of an identical filterbank.:::
% 
%     [f,fs] = greasy;
%     J = 6;
%     [c,info] = dtwfb(f,{'qshift3',J});
%     figure(1);
%     plotwavelets(c,info,fs,'dynrange',90);
%     figure(2);
%     [g,a] = dtwfb2filterbank({'qshift3',J});
%     filterbankfreqz(g,a,1024,'plot','linabs');
%
%   The second example shows a decomposition using a full filterbank tree
%   of depth *J*:::
%
%     [f,fs] = greasy;
%     J = 5;
%     [c,info] = dtwfb(f,{'qshift4',J,'full'});
%     figure(1);
%     plotwavelets(c,info,fs,'dynrange',90);
%     figure(2);
%     [g,a] = dtwfb2filterbank({'qshift4',J,'full'});
%     filterbankfreqz(g,a,1024,'plot','linabs');
%
%   See also: idtwfb plotwavelets dtwfb2filterbank
%
%   References: king02 sebaki05 bayse08

% Author: Zdenek Prusa
% Date:   30.6.2014

complainif_notenoughargs(nargin,2,'DTWFB');

definput.import = {'wfbtcommon'};
flags =ltfatarghelper({},definput,varargin);

% Initialize the wavelet tree structure
dtw = dtwfbinit(dualwt,flags.forder);
    
%% ----- step 1 : Verify f and determine its length -------
[f,Ls]=comp_sigreshape_pre(f,upper(mfilename),0);

% Determine next legal input data length.
L = wfbtlength(Ls,dtw,'per');

% Pad with zeros if the safe length L differs from the Ls.
if(Ls~=L)
   f=postpad(f,L); 
end

%% ----- step 3 : Run computation
[nodesBF, rangeLoc, rangeOut] = treeBFranges(dtw);

c = comp_dtwfb(f,dtw.nodes(nodesBF),dtw.dualnodes(nodesBF),rangeLoc,...
               rangeOut,'per',1);

%% ----- Optionally : Fill info struct ----
if nargout>1
   % Transform name
   info.fname = 'dtwfb';
   % Dual-Tree struct.
   info.wt = dtw;
   % Periodic boundary handling
   info.ext = 'per';
   % Lengths of subbands
   info.Lc = cellfun(@(cEl) size(cEl,1),c);
   % Signal length
   info.Ls = Ls;
   % Ordering of the subbands
   info.fOrder = flags.forder;
   % Cell format
   info.isPacked = 0;
end


