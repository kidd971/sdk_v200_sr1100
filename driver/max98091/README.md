## MAX98091
[Datasheet](https://datasheets.maximintegrated.com/en/ds/MAX98091.pdf)


In master mode, the device uses two integer values (NI and MI) as a multiplier and divider (respectively) to scale PCLK into LRCLK. BCLK is then created either from a PCLK  divider  or  from  an  LRCLK  multiplier  (Table  40).

Based on the oversampling rate selected (OSR), and the configured NI/MI ratio, the output LRCLK frequency is calculated with the following relationship:
- fLRCLK = fPCLK x NI / (MI * OSR)

This expression illustrates that in master mode, the relationship between LRCLK and PCLK frequency (as well as BCLK) is based on an integer ratio. As a result, any cycle
to  cycle  jitter  or  absolute  frequency  variation  in  MCLK is translated first into PCLK and then into LRCLK (and BCLK) based on the selected clock ratios.

**To be compatible with the STM32 SAI, BCLK must be 32*FS.**

### Clock Generation: Manual Ratio Mode
In manual ratio mode, the NI and MI registers are directly programmed to set up the clock ratio.  Manual  ratio  mode  is  only  active  when  the quick Configuration and Exact Integer Modes are disabled. In manual ratio mode, if USE_MI (Table 44) is set to 0, MI is  fixed  at  its  maximum  value  of  0xFFFF(65536) and the programmed value has no effect. For optimal performance (especially with any noninteger PCLK to LRCLK ratio), set USE_MI to 1 and calculate both MI and NI. To calculate the appropriate NI and MI value, use the following method:
1)  Choose the over sampling rate (OSR). If fPCLK < 256 x  fLRCLK,  then  OSR  must  be  set  to  64.  Otherwise, OSR can be set to either 128 or 64. For optimal performance, choose OSR = 128 when possible.
2) Calculate the oversampling frequency using the LRCLK frequency, and the selected oversampling rate:
    - fOSR = fLRCLK x OSR.
3)  Calculate  MI  using  the  prescaled  master  clock  frequency, and the greatest common denominator (GCD) of  the  prescaled  master  clock  frequency  and  the calculated oversampling frequency:
    - MI = fPCLK/GCD(fPCLK, fOSR)
4)  Calculate  NI  using  the  calculated  oversampling frequency and MI value:
    - NI = fOSR x MI/fPCLK

| Fs    | 256*Fs   | OSR | FOSR    | Mi | Ni |
|-------|----------|-----|---------|----|----|
| 96000 | 24576000 | 64  | 6144000 | 2  | 1  |
| 48000 | 12288000 | 128 | 6144000 | 2  | 1  |
| 32000 | 8192000  | 128 | 4096000 | 3  | 1  |
| 24000 | 6144000  | 128 | 3072000 | 4  | 1  |
| 16000 | 4096000  | 128 | 2048000 | 6  | 1  |
| 12000 | 3072000  | 128 | 1536000 | 8  | 1  |
| 8000  | 2048000  | 128 | 1024000 | 12 | 1  |
