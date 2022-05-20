/*! Defines return values returned by OS functions. */

#ifndef __RESULT_H
#define __RESULT_H

/* similar to HRESULT: http://msdn.microsoft.com/en-us/library/windows/desktop/aa378137%28v=vs.85%29.aspx */
#define S_OK             0  /*!< Operation succeeded */
#define E_UNSPECIFIED   -1  /*!< An unknown/unspecified error has occured */

/*! \brief The Function is ready; example: could be waitung for new Data */
#define S_READY          1

/*! \brief The Operation started; example: Aes encryption has started */
#define S_STARTED        2

/*! \brief The operation is running right now, try it later; example: AES
 *         decryptcore is working
 */
#define S_BUSY           3

/*! \brief The operation got stopped due to a normal action ( no ERROR);
 *         example: The RFID Chip stopped scanning
 */
#define S_STOPPED        4

#endif // __RESULT_H
