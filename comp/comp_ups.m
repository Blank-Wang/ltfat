function fups = comp_ups(f,varargin)
%COMP_UPS Upsampling
%   Usage: fups = comp_ups(f,a,type) 
%          fups = comp_ups(f,a,type,'dim',dim) 
%   
%   Input parameters:
%         f     : Input vector/matrix.
%         a     : Upsampling factor.
%         type  : Type of the upsampling.
%         dim   : Direction of upsampling.
%   Output parameters:
%         fups  : Upsampled vector/matrix.
%
%   Upsamples input *f* by a factor *a* (puts *a-1* zeros between data elements)
%   along dimension *dim*. If *dim* is not specified, first non-singleton
%   dimension is used. Parameter *type* (integer from [0:3]) specifies whether the upsampling
%   includes beginning/tailing zeros:
%
%   type=0 (default): Includes just tailing zeros.
%   type=1: No beginning nor tailing zeros.
%   type=2: Includes just begining zeros.
%   type=3: Includes both. 
%
%   Examples:
%   ---------
%
%   The outcome of the default upsampling type is equal to the upsampling performed
%   directly in the frequency domain using repmat:::
%
%      f = 1:4;
%      a = 3;
%      fupsTD = comp_ups(f,a)
%      fupsFD = real(ifft(repmat(fft(f),1,a)))
%


definput.keyvals.dim = [];
definput.keyvals.a = 2;
definput.keyvals.type = 0;
[flags,kv,a,type]=ltfatarghelper({'a','type'},definput,varargin);

% a have to be positive integer
if(a<1)
    a = 1;
end
if(a<0 || rem(a,1)~=0)
    error('%s: Parameter *a* have to be a positive integer.',upper(mfilename));
end
% supported type are 0-3
if(type<0||type>3)
    error('%s: Unsupported upsampling type.',upper(mfilename));
end

if(ndims(f)>2)
    error('%s: Multidimensional signals (d>2) are not supported.',upper(mfilename));
end

%% ----- step 1 : Verify f and determine its length -------
[f,L,Ls,~,dim,permutedsize,order]=assert_sigreshape_pre(f,[],kv.dim,upper(mfilename));

if(type==0)
  % Include just tailing zeros.
  fups=zeros(a*Ls,size(f,2));    
  fups(1:a:end,:)=f; 
elseif(type==1)
  % Do not include beginning nor tailing zeros.
  fups=zeros(a*Ls-(a-1),size(f,2));    
  fups(1:a:end,:)=f;    
elseif(type==2)
  % Include just beginning zeros.
  fups=zeros(a*Ls,size(f,2));    
  fups(a:a:end,:)=f;  
elseif(type==3)
  % Include both beginning and tailing zeros.
  fups=zeros(a*Ls+a-1,size(f,2));    
  fups(a:a:end,:)=f;   
end

permutedSizeAlt = size(fups);
fups=assert_sigreshape_post(fups,dim,permutedSizeAlt,order);
