function c = comp_ufwt(f,h,a,J,scaling)
%COMP_UFWT Compute Undecimated DWT
%   Usage:  c=comp_ufwt(f,h,J,a);
%
%   Input parameters:
%         f     : Input data - L*W array.
%         h     : Analysis Wavelet filters - cell-array of length *filtNo*.
%         J     : Number of filterbank iterations.
%         a     : Subsampling factors - array of length *filtNo*.
%
%   Output parameters:
%         c     : L*M*W array of coefficients, where M=J*(filtNo-1)+1.
%


% This could be removed with some effort. The question is, are there such
% wavelet filters? If your filterbank has different subsampling factors after first two filters, please send a feature request.
assert(a(1)==a(2),'First two elements of a are not equal. Such wavelet filterbank is not suported.');

% For holding the time-reversed, complex conjugate impulse responses.
filtNo = length(h);
% Optionally scale the filters
h = comp_filterbankscale(h(:),a(:),scaling);
%Change format to a matrix
%hMat = cell2mat(cellfun(@(hEl) conj(flipud(hEl.h(:))),h(:)','UniformOutput',0));
hMat = cell2mat(cellfun(@(hEl) hEl.h(:),h(:)','UniformOutput',0));

%Delays
%hOffset = cellfun(@(hEl) 1-numel(hEl.h)-hEl.offset,h(:));
hOffset = cellfun(@(hEl) hEl.offset,h(:));

% Allocate output
[L, W] = size(f);
M = J*(filtNo-1)+1;
c = zeros(L,M,W,assert_classname(f,hMat));

ca = f;
runPtr = size(c,2) - (filtNo-2);
for jj=1:J
    % Zero index position of the upsampled filters.
    offset = a(1)^(jj-1).*(hOffset);
    % Run filterbank.
    ca=comp_atrousfilterbank_td(ca,hMat,a(1)^(jj-1),offset);
    % Bookkeeping
    c(:,runPtr:runPtr+filtNo-2,:)=ca(:,2:end,:);
    ca = squeeze(ca(:,1,:));
    runPtr = runPtr - (filtNo - 1);
end
% Saving final approximation coefficients.
c(:,1,:) = ca;

