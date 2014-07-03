function wt = nat2freqOrder(wt,varargin)
%NAT2FREQORDER Natural To Frequency Ordering
%   Usage:  wt = nat2freqOrder(wt);
%
%   Input parameters:
%         wt    : Structure containing description of the filter tree.
%
%   Output parameters:
%         wt    : Structure containing description of the filter tree.
%
%   `nat2freqOrder(wt)` Creates new wavelet filterbank tree definition
%   with permuted order of some filters for purposes of the correct frequency
%   ordering of the resultant identical filters and coefficient subbands. 
%   For definition of the structure see |wfbinit| and |dtwfbinit|.
%
%   `nat2freqOrder(wt,nodes)` does the same but works only with nodes
%   listed in *nodes*.
%
%   REMARK: The function is self invertible.
%
%   See also: wfbtinit,  wfbtmultid, nodeBForder
%

complainif_notenoughargs(nargin,1,'NAT2FREQORDER');

do_rev = ~isempty(varargin(strcmp('rev',varargin)));

if do_rev
    %Remove the 'rev' flag from varargin
    varargin(strcmp('rev',varargin)) = [];
end

if isempty(varargin)
   % Work with the whole tree
   treePath = nodeBForder(0,wt);
   %skip root
   treePath = treePath(2:end);
else
   % Work with specified nodes only
   nodes = varargin{1}; 
   % Omit root
   nodes(wt.parents(nodes)==0) = [];
   % Use the rest 
   treePath = nodes;
end

% Dual-tree complex wavelet packets require some more tweaking.
 if isfield(wt,'dualnodes')
    locIdxs = arrayfun(@(tEl) find(wt.children{wt.parents(tEl)}==tEl,1),treePath);
    treeNodes = treePath(rem(locIdxs,2)~=1);
    % Root was removed so the following will not fail
    jj = treeNodes(find(treeNodes(wt.parents(wt.parents(treeNodes))==0),1));
 
    while ~isempty(jj) && ~isempty(wt.children{jj})
        sanChild = postpad(wt.children{jj},numel(wt.nodes{jj}.g));
        % Reverse child nodes
        wt.children{jj} = sanChild(end:-1:1); 
 
        if do_rev
          jj = wt.children{jj}(1);
        else
          jj = wt.children{jj}(end);
        end
        
    end
 
 end


% Get loc. index of nodes
locIdxs = arrayfun(@(tEl) find(wt.children{wt.parents(tEl)}==tEl,1),treePath);

% Get only nodes connected to a high-pass output
treeNodes = treePath(rem(locIdxs,2)~=1);

for nodeId=treeNodes
       % now for the filter reordering
       wt.nodes{nodeId}.g = wt.nodes{nodeId}.g(end:-1:1);
       wt.nodes{nodeId}.h = wt.nodes{nodeId}.h(end:-1:1);
       wt.nodes{nodeId}.a = wt.nodes{nodeId}.a(end:-1:1);
       
        % Do the same with the dual tree if it exists
        if isfield(wt,'dualnodes')
            wt.dualnodes{nodeId}.g = wt.dualnodes{nodeId}.g(end:-1:1);
            wt.dualnodes{nodeId}.h = wt.dualnodes{nodeId}.h(end:-1:1);
            wt.dualnodes{nodeId}.a = wt.dualnodes{nodeId}.a(end:-1:1);
        end
end












