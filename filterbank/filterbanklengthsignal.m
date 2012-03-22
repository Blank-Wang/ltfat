function L=filterbanklengthsignal(Ls,a);
%FILTERBANKLENGTHSIGNAL  Filterbank length from signal
%   Usage: L=filterbanklengthsignal(Ls,a);
%
%   `filterbanklengthsignal(Ls,a)` returns the length of a filterbank with
%   time shifts *a*, such that it is long enough to expand a signal of
%   length *Ls*.
%
%   If the filterbank length is longer than the signal length, the signal will be
%   zero-padded by |filterbank|_ or |ufilterbank|_.
%
%   If instead a set of coefficients are given, call |filterbanklengthcoef|_.
%
%   See also: filterbank, filterbanklengthcoef

if ~isnumeric(a)
  error('%s: a must be numeric.',upper(callfun));
end;

if ~isvector(a) || any(a<=0)
  error('%s: "a" must be a vector of positive numbers.',upper(callfun));
end;

lcm_a=a(1);
for m=2:length(a)
  lcm_a=lcm(lcm_a,a(m));
end;

L=ceil(Ls/lcm_a)*lcm_a;

L