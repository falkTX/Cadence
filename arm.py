#!/usr/bin/env python3

import re
import os

generic = {
    '7' : 'arm7',
    '8' : 'arm8',
    '9' : 'arm9tdmi',
    'a' : 'arm10e',
    'b' : 'mpcore'
    }

specific = {
    '810' : 'arm810',
    '8xx' : 'arm8',
    '920' : 'arm920t',
    '922' : 'arm922t',
    '946' : 'arm946e-s',
    '926' : 'arm926ej-s',
    '966' : 'arm966e-s',
    '968' : 'arm968e-s',
    '940' : 'arm940e',
    '9xx' : 'arm9tdmi',
    'a20' : 'arm1020e',
    'a22' : 'arm1022e',
    'a26' : 'arm1026ej-s',
    'axx' : 'arm10e',
    'b36' : 'arm1136j-s',
    'b56' : 'arm1156t2-s',
    'b76' : 'arm1176jz-s',
    'bxx' : 'mpcore',
    'c05' : 'cortex-a5',
    'c07' : 'cortex-a7',
    'c08' : 'cortex-a8',
    'c09' : 'cortex-a9',
    'c0d' : 'cortex-a12',
    'c0f' : 'cortex-a15',
    'c0e' : 'cortex-a17',
    'd01' : 'cortex-a32',
    'd04' : 'cortex-a35',
    'd03' : 'cortex-a53',
    'd05' : 'cortex-a55',
    'd07' : 'cortex-a57',
    'd08' : 'cortex-a72',
    'd09' : 'cortex-a73',
    'd0a' : 'cortex-a75',
    'c14' : 'cortex-r4',
    'c15' : 'cortex-r5',
    'c17' : 'cortex-r7',
    'c18' : 'cortex-r8',
    'd21' : 'cortex-m33',
    'd20' : 'cortex-m23',
    'c20' : 'cortex-m7',
    'c24' : 'cortex-m4',
    'c23' : 'cortex-m3',
    'c21' : 'cortex-m1',
    'c20' : 'cortex-m0',
    'c60' : 'cortex-m0plus'
}


class CPUInfo(object):

    ARM='arm'
    Intel='x86'
    Generic='generic'


    def __init__(self):
        self.architecture=os.uname().machine
        if re.match('arm',self.architecture): self.chip=CPUInfo.ARM
        elif re.match('x86',self.architecture): self.chip=CPUInfo.Intel
        else : self.chip=CPUInfo.Generic
        
        with open('/proc/cpuinfo','r') as proc:
            lines=[l for l in proc]

        self.lines=[re.sub('\s+',' ',l) for l in lines]
        f=[re.match('(?:Features|flags) . ([\w ]+)$',l) for l in self.lines]
        f=[m.groups(0)[0] for m in f if m is not None]
        self.features=set()
        for line in f : self.features.update(line.strip().split(' '))

        if self.chip==CPUInfo.ARM:
            c=[re.match('CPU part\s:\s+0x([0-9a-f]+)',l) for l in self.lines]
            self.cpu={m.groups(0)[0] for m in c if m is not None}.pop()
            self.model=specific.get(self.cpu,generic.get(self.cpu,'generic'))
        else:
            self.cpu=None
            self.model='generic'


    def __getitem__(self,key):
        return key in self.features

    @property
    def fpuFlags(self):
        if self.chip==CPUInfo.Intel:
            if self['sse'] or self['sse2']:
                return ['-msse','-mfpmath=sse']
            return []
        elif self.chip==CPUInfo.ARM:
            f=[]
            if self['neon']: f=['-mfpu=neon-fp-armv8','-mneon-for-64bits']
            elif self['vfpv4']: f=['-mfpu=neon-vfpv4']
            elif self['vfpv3']: f=['-mfpu=neon-vpfv3']
            elif self['vfpv3d16']: f=['-mfpu=vfpv3-d16']
            elif self['vfp'] or self['vfpv2'] or self['vfpd32']: f=['-mfpu=vfp']
            f.append('-mfloat-abi=hard')
            return f
        else:
            return []        

    @property
    def modelFlags(self):
        if self.chip==CPUInfo.ARM:
            if self.model=='generic': return ['-mtune=generic']
            else: return ['-mcpu={}'.format(self.model),'-mtune={}'.format(self.model)]
        else:
            return ['-mtune=generic']
        
    @property
    def flags(self):
        f=self.modelFlags
        f.extend(self.fpuFlags)
        return f

try:
    c=CPUInfo()
    print(' '.join(c.flags))
except Exception as e:
    print('Error : {}'.format(e))

                
        

    
