#ifndef _ERROR_HANDLING_API_
#define _ERROR_HANDLING_API_


#define         ReportError()               _ReportError(__func__, __LINE__, __FILE__)


void            RepportErrorByLed           (void);
void            ClearErrorLed               (void);
void            _ReportError                (const char *Function, int Line, char *File);
void            ToggleHeartBeat             (void);


#endif /* _ERROR_HANDLING_API_ */
